#include <vector>
#include <streambuf>
#include <algorithm>
#include <ostream>

class MultiStream: public std::ostream{
    struct MultiBuffer: public std::streambuf{
        void addBuffer(std::streambuf* buf){
            bufs.push_back(buf);
        }
        virtual int overflow(int c){
            for (auto buffer : bufs){
                buffer->sputc(c);
            }
            return c;
        }

        private:
            std::vector<std::streambuf*> bufs;

    };  
    MultiBuffer myBuffer;
    public: 
        MultiStream(): std::ostream(NULL){
            std::ostream::rdbuf(&myBuffer);
        }   
        void linkStream(std::ostream& out){
            out.flush();
            myBuffer.addBuffer(out.rdbuf());
        }
};