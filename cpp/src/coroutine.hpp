# if __has_include(<experimental/coroutine>)
#       include <experimental/coroutine>
namespace std {
    namespace suspend_always = experimental::suspend_always ;
    namespace suspend_never = experimental::suspend_never ;
    namespace coroutine_handle = experimental::coroutine_handle ;
}
# else
#   include <coroutine>
# endif