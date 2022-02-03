#ifndef BUFFER_HPP_CXX
#define BUFFER_HPP_CXX
#include<stdlib.h>
#include<assert.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<unistd.h>
class buffer {
    private:
       size_t _size;
    public:
        char* buf;
        
    public:
        explicit buffer(size_t size = 1024) {
            buf = new char[size];
            _size = size;
        } 
        buffer(buffer& other) {
            if(this == &other)
                return;
            delete[] buf;
            int n = other._size;
            buf = new char[n];
            _size = n;
            for(int i = 0; i < n; i++) {
                buf[i] = other.buf[i];
            }

        }
        ~buffer() {
            delete[] buf;
        }     
        buffer& operator=(buffer& other) {
            if(this == &other)
                return *this;
            delete[] buf;
            int n = other._size;
            buf = new char[n];
            _size = n;
            for(int i = 0; i < n; i++) {
                buf[i] = other.buf[i];
            }
            return *this;
        }
        char& operator[](int pos) {
            assert(pos < this->_size);
            return buf[pos];   
        }
        size_t size() const {
            return _size;
        }
        void BufToFile(const char* filename, int flags, mode_t mode, int begin = 0, int end = -1) {
            if(end == -1)
                end = _size; 
            int fd = open(filename, flags, mode);
            assert(fd >= 0);
            write(fd, buf + begin, end - begin);
            close(fd);
        }  

};

#endif
