
#ifndef SCM_DATA_SHARED_ARRAY_H_INCLUDED
#define SCM_DATA_SHARED_ARRAY_H_INCLUDED

namespace data {

template <typename T>
	class shared_array {
	public:
		shared_array(T* v) : v(v), rc(new int()) { *rc = 1; }
		shared_array(const shared_array<T>& rhs) : v(rhs.v), rc(rhs.rc) { ++(*rc); }
		~shared_array() { decr(); }

		shared_array<T>& operator=(const shared_array<T>& rhs) {
			if (this->v != rhs.v) {
				decr();
				this->v  = rhs.v;
				this->rc = rhs.rc;
				++(*rc);
			}
			return *this;
		}

		T* get() const { return this->v; }
		T* ptr() const { return this->v; }
	private:
		T*   v;
		int* rc;

		void decr() {
			if (0 == --(*rc)) {
				delete[] v;
				delete rc;
			}
		}
	};

}

#endif
