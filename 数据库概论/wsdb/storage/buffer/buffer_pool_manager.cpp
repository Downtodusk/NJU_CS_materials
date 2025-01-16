/*------------------------------------------------------------------------------
 - Copyright (c) 2024. Websoft research group, Nanjing University.
 -
 - This program is free software: you can redistribute it and/or modify
 - it under the terms of the GNU General Public License as published by
 - the Free Software Foundation, either version 3 of the License, or
 - (at your option) any later version.
 -
 - This program is distributed in the hope that it will be useful,
 - but WITHOUT ANY WARRANTY; without even the implied warranty of
 - MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 - GNU General Public License for more details.
 -
 - You should have received a copy of the GNU General Public License
 - along with this program.  If not, see <https://www.gnu.org/licenses/>.
 -----------------------------------------------------------------------------*/

//
// Created by ziqi on 2024/7/17.
//
#include "buffer_pool_manager.h"
#include "replacer/lru_replacer.h"
#include "replacer/lru_k_replacer.h"

#include "../../../common/error.h"

namespace wsdb {

BufferPoolManager::BufferPoolManager(DiskManager *disk_manager, wsdb::LogManager *log_manager, size_t replacer_lru_k)
    : disk_manager_(disk_manager), log_manager_(log_manager)
{
  if (REPLACER == "LRUReplacer") {
    replacer_ = std::make_unique<LRUReplacer>();
  } else if (REPLACER == "LRUKReplacer") {
    replacer_ = std::make_unique<LRUKReplacer>(replacer_lru_k);
  } else {
    WSDB_FETAL("Unknown replacer: " + REPLACER);
  }
  // init free_list_
	for (frame_id_t i = 0; i < static_cast<int>(BUFFER_POOL_SIZE); i++) {
		free_list_.push_back(i);
	}
}

auto BufferPoolManager::FetchPage(file_id_t fid, page_id_t pid) -> Page * {
	std::lock_guard<std::mutex> lock(latch_);

	auto page_lookup_key = fid_pid_t{fid, pid};

	// 检查页面是否已经在缓冲池中
	auto it = page_frame_lookup_.find(page_lookup_key);
	// 页面不在缓冲池中
	if (it == page_frame_lookup_.end()) {
		frame_id_t frame_id = GetAvailableFrame();
		UpdateFrame(frame_id, fid, pid);
		return frames_[frame_id].GetPage();
	}

	frame_id_t frame_id = it->second;
	Frame &frame = frames_[frame_id];
	frame.Pin();
	replacer_->Pin(frame_id);
	return frame.GetPage();
}

auto BufferPoolManager::UnpinPage(file_id_t fid, page_id_t pid, bool is_dirty) -> bool
{
  std::lock_guard<std::mutex> lock(latch_);

	auto it = page_frame_lookup_.find({fid, pid});
	if (it == page_frame_lookup_.end()) {
		return false;
	}
	frame_id_t frame_id = it->second;
	Frame& frame = frames_[frame_id];
	if (!frame.InUse()) {
		return false;
	}

	frame.Unpin();
	if (!frame.InUse()) {
		replacer_->Unpin(frame_id);
	}

	if (is_dirty) {
		frame.SetDirty(true);
	}
	return true;
}

auto BufferPoolManager::DeletePage(file_id_t fid, page_id_t pid) -> bool
{
	std::lock_guard<std::mutex> lock(latch_);
	auto it = page_frame_lookup_.find({fid, pid});
	if (it == page_frame_lookup_.end()) {
		return true;
	}
	frame_id_t frame_id = it->second;
	Frame &frame    = frames_[frame_id];
	if (frame.InUse()) {
		return false;
	}
	if (frame.IsDirty()) {
		disk_manager_->WritePage(fid, pid, frame.GetPage()->GetData());
	}
	frame.Reset();
	free_list_.push_back(frame_id);
	replacer_->Unpin(frame_id);
	page_frame_lookup_.erase(it);
	// printf("Avaliable page : %ld\n", free_list_.size());
	return true;
}

auto BufferPoolManager::DeleteAllPages(file_id_t fid) -> bool
{
	//std::lock_guard<std::mutex> lock(latch_);
	bool all_deleted = true;
	for (auto it = page_frame_lookup_.begin(); it != page_frame_lookup_.end();) {
	if (it->first.fid == fid) {
		auto next_it = std::next(it);
		bool deleted = DeletePage(fid, it->first.pid);
		if (!deleted) {
			all_deleted = false;
			break;
		}
			it = next_it;
		} else {
			++it;
		}
	}
	return all_deleted;
}

auto BufferPoolManager::FlushPage(file_id_t fid, page_id_t pid) -> bool
{
	std::lock_guard<std::mutex> lock(latch_);
	auto frame = GetFrame(fid, pid);
	if (frame == nullptr) {
		return false;
	}
	if (frame->IsDirty()) {
		disk_manager_->WritePage(fid, pid, frame->GetPage()->GetData());  // 将脏页写入磁盘
		frame->SetDirty(false);
	}
	return true;
}

auto BufferPoolManager::FlushAllPages(file_id_t fid) -> bool
{
	bool all_flushed = true;
	for(auto it = page_frame_lookup_.begin(); it != page_frame_lookup_.end(); ++it) {
		if (it->first.fid == fid) {
			bool flushed = FlushPage(fid, it->first.pid);
			if (!flushed) {
				all_flushed = false;
			}
		}
	}
	return all_flushed;
}

auto BufferPoolManager::GetAvailableFrame() -> frame_id_t {
	if (!free_list_.empty()) {
		frame_id_t frame_id = free_list_.front();
		free_list_.pop_front();
		return frame_id;
	}
	frame_id_t frame_id;
	if (replacer_->Victim(&frame_id)) {
		return frame_id;
	}
	WSDB_THROW(WSDB_NO_FREE_FRAME, "error: no free frame");
}

void BufferPoolManager::UpdateFrame(frame_id_t frame_id, file_id_t fid, page_id_t pid) {
	Frame &frame = frames_[frame_id];
	Page  *page  = frame.GetPage();
	if (frame.IsDirty()) {
		disk_manager_->WritePage(page->GetTableId(), page->GetPageId(), frame.GetPage()->GetData());
	}
	page_frame_lookup_.erase({page->GetTableId(), page->GetPageId()});
	frame.Reset();

	disk_manager_->ReadPage(fid, pid, frame.GetPage()->GetData());
	frame.GetPage()->SetTablePageId(fid, pid);

	frame.Pin();
	replacer_->Pin(frame_id);

	page_frame_lookup_[{fid, pid}] = frame_id;
}

auto BufferPoolManager::GetFrame(file_id_t fid, page_id_t pid) -> Frame *
{
	const auto it = page_frame_lookup_.find({fid, pid});
	return it == page_frame_lookup_.end() ? nullptr : &frames_[it->second];
}

}  // namespace wsdb