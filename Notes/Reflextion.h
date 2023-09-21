pointer-to-member 

struct X
{
   void f() {}
   void g() {}
};

typedef void (X::*pointer)();
//ok, let's take a pointer and assign f to it.
pointer somePointer = &X::f;
//now I want to call somePointer. But for that, I need an object
X x;
//now I call the member function on x like this
(x.*somePointer)(); //will call x.f()
//now, suppose x is not an object but a pointer to object
X* px = new X;
//I want to call the memfun pointer on px. I use ->*
(px ->* somePointer)(); //will call px->f();

SFINAE => Substitution Failure is Not A Error 

template <std::size_t N>
constexpr std::integral_constant<std::size_t, N> array_length_helper(auto (&&)[N]);

template <typename T>
static constexpr auto array_length = decltype(array_length_helper(std::declval<T>()))::value;

// if expression is a pointer to a polymorphic type, and new-type is a pointer to void, the result
// is a pointer to the most derived object pointed or reference by expression 
// so in below dynamic_cast cast to an most derived (if not this expression fail and all back ot next)
template<typename T>
std::true_type check_expression(decltype(sizeof(dynamic_cast<void*>(std::declval<T*>() ) )));

template <typename T>
std::false_type check_expression(...);

template <typename T>
static constexpr auto is_polymorphic= (decltype(check_expression<T>(0))::value != 0);