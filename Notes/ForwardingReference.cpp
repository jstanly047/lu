/*
Forwaring reference represent..
    1. and lvalue reference if they are initialized by an lvalue
    2. an rvalue reference if the are initialized by an rvalue

Rvalue reference are forwarding references if they
    1. involve in type deduction (not class template)
    2. appear in exactly the form of T&& or auto&&

template <typename T>
void f(T&& x )

auto&& var = var1;


reference collapsing rule
&  & -> &
&& &-> &
&  && -> &
&& &&-> &&

*/

/*
Perfect Forwarding
Refere Forwaring Ref
*/
class Widget
{

};

template<typename T, typename Arg>
myUnique_ptr<T> my_make_unique(Arg arg){
    return myUnique_ptr<T>(new T(arg));
}

my_make_unique<int>(1);
my_make_unique<Widget>(w); // Expensive extra copy

template<typename T, typename Arg>
myRUnique_rtr<T> my_make_unique_r(Arg& arg){
    return myRUnique_rtr<T>(new T(arg));
}

// my_make_unique_r<int>(1)  compiler error
template<typename T, typename Arg>
myCRUnique_rtr<T> my_make_unique_cr(Arg const& arg){
    return myCRUnique_rtr<T>(new T(arg));
}

/*
struct Example{ Example(int & );};
int i{1};
std::my_make_unique_cr<Example>(i); // compiler error
*/

template<typename T, typename Arg>
myPerfectUnique_ptr<T> my_make_unique_perfect(Arg&& arg){
    return myPerfectUnique_ptr<T>(new T(arg));
}
/*
 Above work for all cases but it has problem
 when a rvalue passed to the function it becomes lvalue since  arg is lvalue (has name)
*/

template<typename T>
T&& stdForward(std::remove_reference_t<T>& t) noexcept
{
    return static_cast<T&&>(t);

}

template<typename T, typename... Arg>
unique_ptr<T> make_unique(Arg&&... args){
    return unique_ptr<T>(new T(std::forward<Arg>(args)...);
}




struct Person{
    Person(const std::string& name);
    template<typename T> Person(T&&);
};

Person p1("Bjorne"); // call ctor (2)
std::string name("herb"); 
Person p2(name); // call ctor (2) since name is not const
Person p3(p1); // call ctor (2)

void f(Widget&) (1)
void f(const Widget& ) (2)
void f(Widget && ); (3)
void f(const Widget&& ); (4)

template <typename T>
void f(T&& ); (5)

template<typename T>
void f(const T&&); (6)


//Case 1
Widget w;
f(w);
(1), (5), (2)


//case 2
const Widget w;
f(w)
(2), (5)

//case 3
Widget getWidget();
f(getWidget);
(3), (5), (4), (6), (2)

// case 4
const Widget getWidget();
f(getWidget);
(4), (6), (5), (4)

/*
Avoid overloading on universal(forward) reference
*/

class A
{
public:
    template<typename T>

    A(T&& t): b_(std::forward<T>(t)){}
    /*
        Wrong to have
        A(T&& t):b_(std::move(t)){}
    */
private:   
    B b_;
};


template<typename T>
class A1
{
public:
    A1(T&& t):b_(std::move(t)){}
    /*
        Wrong
        A1(T&& t): b_(std::forward<T>(t)){}
        since template on class level so this not forward reference
    */
private:
    B b_;
};

class A2
{
public:
    template<typename T>
    A2(T&& t): b_(t),
             c_(std::forward<T>(t))
            {}
    /*
        Wrong to have
        A2(T&& t): b_(std::forward<T>(t)),
                  c_(std::forward<T>(t))
            {}

        this move twice on t, so second move is moving a invalid state
    */
private:   
    B b_;
    C c_;
};


class A3
{
public:
    template<typename T1, typename T2>
    A3(T1&& t1, T2&& t2): b_(std::forward<T>(t1)),
             c_(std::forward<T>(t2))
            {}
    /*
        A3(std::move(t), std::move(t)) is a coding error
        so no need to check for same ref
    */
private:   
    B b_;
    C c_;
};


template<typename T, typename... Args>
unique_ptr<T> create(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
    /*
    Below is an antipattern since it restrict RVO

    auto uptr(std::make_unique<T>(std::forward<Args>(args)...));
    return std::move(uptr);
    */
}
