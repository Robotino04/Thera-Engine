#include <stdexcept>


namespace Thera::Utils{
class NotImplementedException : public std::logic_error{
    public:
        NotImplementedException(): std::logic_error("Function not yet implemented") {};
        NotImplementedException(std::string const& msg): std::logic_error(msg) {};
};
}