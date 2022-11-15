#include <iostream>
#include <type_traits>
#include <memory>
#include <utility>

template<typename T>
std::remove_reference_t<T>&& myMove(T&& t) noexcept
{
    return static_cast<std::remove_reference_t<T>&&>(t);
}

/* Rule Of Zero : If you can avoid defining default operations, do */
class Widget
{
public:
    /*
        Widget v{};
        Widget v2 = v;
    */
    Widget(const Widget& rhs) = default;
    /*
        Widget v1{};
        Widget v2{};
        v1 = v2;
    */
    Widget& operator=(const Widget& rhs) = default;

    /*
        Widget creatWidget()
        {
            Widget v;
            ......
            return v
        }

        Widget v2 = Widget{};
        Widget v3 = createWidget(); // RVO has priority
        Widget v4 = std::move(v2);
    */
    Widget(Widget&& rhs) = default;
    Widget& operator=(Widget&& rhs) = default;


private:
    int i{0};
    std::string s{};
    std::unique_ptr<int> ptr{};
    /*
     When you have a unique_ptr as memebr then this class isn't copyable. Since ‘Widget::Widget(const Widget&)’ is implicitly deleted because the default definition would be ill-formed
     unique_ptr(const unique_ptr&) = delete; // in the standed libarary
    */
    
};

class WidgetPtr
{
public:

    WidgetPtr(const WidgetPtr& rhs): i(rhs.i), s(rhs.s){
        ptr = new int(*rhs.ptr);
    }
    WidgetPtr& operator=(const WidgetPtr& rhs){
        // Self assignment check, default assignment operator doesn't check self assignment
        if (&rhs == this)
        {
            return *this;
        }

        delete ptr;

        i = rhs.i;
        s = rhs.s;
        ptr = new int(*rhs.ptr);
        return *this;
    }

    /* Wrong
        WidgetPtr(WidgetPtr&& rhs): i (rhs.i), s (rhs.s), ptr(rhs.prt) {
            rhs.ptr = nullptr;
        }

        1. s (rhs.s) // call copy instead of move since rsh.s has name (lvalue)

        2. without noexcept compiler tent do call copy constructor instead of move 
    */

    WidgetPtr(WidgetPtr&& rhs) noexcept: i (std::move(rhs.i)), 
                                s (std::move(rhs.s)),
                                ptr (std::exchange(rhs.ptr, nullptr)) 
    {
        /* rhs.i = 0 // Purely optional, not done by default
           After move original should be in invalid stated or undefine
        */

    }

    WidgetPtr& operator=(WidgetPtr&& rhs) noexcept{
        delete ptr; //**********
        i = std::move(rhs.i);
        s = std::move(rhs.s);
        ptr = std::exchange(rhs.ptr, nullptr);
        /*  std::swap(pi, rhs.ptr) 
            swap is smart rather than calling delete and exchange

            Self assignment check not required since it can happend when
            w = std::move(w)
            std::swap(w, w)
            both are programmer's mistake
        */ 
        return *this;
    }


private:
    int i{0};
    std::string s{};
    int* ptr{};
};

/*
1. Default move operation are generated
        if no copy operation or destructor is user-defined
2. The default copy operation are generated
        if no move operation user-defined
Note: =default and =delete count as user-defined

above rule is complex so simply we can follow below rule,
   Rule of Five: If you define or =delete andy default operation, define or =delete all of them
            - Destructor, copy, move , assignment, move assignment 
   (with default constructor Rule of Six)
*/

int main()
{
    return 0;
}
