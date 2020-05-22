#ifndef _BASE_ENUM_WRAP_HPP_
#define _BASE_ENUM_WRAP_HPP_

#include <vector>
#include <sstream>
#include <cassert>


namespace base {
    struct EnumClassHash
    {
        template <typename T>
        std::size_t operator()(T t) const
        {
            return static_cast<std::size_t>(t);
        }
    };

    template <class ENUM, class LITERAL>
    class EnumWrap {
    public:
        EnumWrap(std::vector<LITERAL> enum_vec) : enum_vec_(std::move(enum_vec)) {}

        ENUM literal2enum(const std::string &value) {
            for (size_t i = 0; i < enum_vec_.size(); ++i) {
                if (value.compare(enum_vec_[i]) == 0) {
                    return static_cast<ENUM>(i);
                }
            }
            throw std::runtime_error("Can't match any enum name, option is " + to_string());
        }

        std::string enum2literal(ENUM value) {
            auto i = enum2index(value);
//            PPK_ASSERT_ERROR(static_cast<size_t>(i) < enum_vec_.size(), "out of range, index=%d, option is %s", static_cast<int>(i), to_string().c_str());
            assert(static_cast<size_t>(i) < enum_vec_.size());
            return enum_vec_[i];
        }

        auto enum2index(ENUM value)
        -> typename std::underlying_type<ENUM>::type {
            return static_cast<typename std::underlying_type<ENUM>::type>(value);
        }
        std::string to_string() {
            std::ostringstream oss;
            oss << "[ ";
            for (size_t i = 0; i < enum_vec_.size(); ++i) {
                if (i >= 1) {
                    oss << ", ";
                }
                oss << enum_vec_[i];
            }
            oss << " ]";
            return oss.str();
        }
    private:
        std::vector<LITERAL> enum_vec_;
    };

} /* base */


#endif /* end of include guard: _BASE_ENUM_WRAP_HPP_ */

