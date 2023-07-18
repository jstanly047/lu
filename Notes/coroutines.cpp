#include <coroutine>


struct ReturnType
{
    
    struct promise_type{}
};

/*
        promise_type is automatically generated for compiler
        This the center point of coroutine
*/
struct  promise_type
{
    /*
        Compiler implicitly call this function when coroutine starts executing.
        Up to the promise_type to construct and return back to the caller
    */
    ReturnType get_return_object{ return {}; }

    /*
        handle co_return 
    */
    void return_void() {}

    /*
        exit the coroutine on exception
    */
    void unhandled_exception()

    /*

    */
   std::suspend_always final_suspend() noexcept { return {} }
   std::suspend_never  initial_suspend() { return {} }
};


/*
 Awaitable are the opportunity to suspense of function and hand the control to caller
*/

struct Awaitable
{
    /* check weather we need to wait or not. On true it doesn't wait*/
    bool await_read();
    /* called before suspensions  of call*/
    void await_suspend(std::coroutine_handle<>);
    /* called before wake up of coroutine */
    void await_resume();
}

Example:
struct SuspendAlways{
    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<>) {}
    void await_resume() {}
}


struct  std::coroutine_handle<promise_type>
{
    /* resume the suspended call */
    void resume() const;
    /* Delete the coroutine state generated  */
    void destroy() const;
};

Template to start:

struct ReturnType / std::coroutine_traits<ReturnType, ...>
{
    struct promise_type
    {
        promise_type(T...); // opt
        ReturnType get_return_object();
        std::suspend_always initial_suspend();
        //-------------start (UP) / (DOWN) Shutdown --------------
        void return_value(T); / void return_void();
        std::suspend_always final_suspend() noexcept;
    }
}

* The bridge between the caller (ReturnType) and coroutine (Awaitable) is done using promise (coroutine_handle)


struct promise_type
{
    ReturnType get_return_object()
    {
        return ReturnType{ std::coroutine_handle<promise>::from_promise(*this) };
    }
};

struct ReturnType
{
    std::coroutine_handle<promise_type> handle;
    ReturnType(std::coroutine_handle<promise_type> h) : handle(h) {}
    void resume() { handle.resume(); }
}