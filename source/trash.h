#ifndef trash_h_
#define trash_h_

class trash_type {
};

template <typename t>
inline static trash_type& operator << (trash_type& can, t* &ptr) {
	if (ptr != nullptr)
		delete ptr;
	ptr = nullptr;
	return can;
}

extern trash_type trash;
// Allows you to delete and set to nullptr, pointers with a shift operator like this:
// trash << deadline_work_ptr_ << message_work_ptr_ << write_version_work_ptr_ << pingpong_work_ptr_ << connect_work_ptr_;

#endif
