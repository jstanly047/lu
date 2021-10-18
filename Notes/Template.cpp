

/****************** Alias templates*/
typedef std::vector<int> myvect_int;
using myvec_double=std::vector<double>;
template<typename T> using myvect = std::vector<T>;


static_assert(is_same_v<myvect_int, std::vector<int>>);
static_assert(is_same_v<myvec_double, std::vector<double>>);
static_assert(is_same_v<myvec<float>, std::vector<float>>);

/****************** type deduction*/
short f();
int x=23;
int y = std::max(f(), x); // error no maching function call 
/*
    template<typename T>
    max(T t1, T t2)
*/


int y = std::max<int>(f(), x); // OK...


template<typename T, typename U>
void add(T x, U y)
{
    puts(__PRETTY_FUNCTION__);
}

add<int,int>('x', 3.1); // [T =int , U =int ]
add<int>('x',3.1);      // [T =int , U =double ]
add<>('x', 3.1);        // [T =char , U =double ]
add('x', 3.1);          // [T =char , U =double ]

template<typename T>
void add()
{
    puts(__PRETTY_FUNCTION__);
}

add<int>(); //OK
add<>(); // error, connot infer it
add();  // error, connot infer it

template<typename T = char*>
void add()
{
    puts(__PRETTY_FUNCTION__);
}
add<int>(); //OK
add<>(); // OK
add(); // OK

template<typename T>
void f(T& t)
{

}

int i = 42;
f(static_cast<int&>(i)); // OK
f(static_cast<int&&>(i)); // Error
f(static_cast<const int&>(i)); // OK
f(static_cast<const int&&>(i)); //OK 
/*
 rvalue is kind of const of lvalue, C+03 instead of rvalue we had const of lvalue
 if i have rvalue and i have function expecting const lvalue that will work
*/

/**********template specialization********/
//case1
template<typename T>
struct is_void{
    static constexpr bool value = false;
};

template<>
struct is_void<void>{
    static constexpr bool value = false;
};

is_void<int>::value // 0
is_void<void>::value //1

// case2
template<typename T>
T abs(T x){

}

template<>
abs(int x){

}

/**********partial specialization********/
template<typename T>
constexpr bool is_array = false;

template<typename Tp> // partial
constexpr bool is_array<Tp[]> = true;

template<typename Tp, int N> // partial 
constexpr bool is_array<Tp[N]> = true;

template<> // full specialization 
constexpr bool is_array<void> = false;




//Example class
template<typename T> class A;

template<> class A<void>; // full

template<typename Tp> class A<Tp*>; //partial

template<typename Tp> class A<Tp**>; //partial

// Wrong connect partially specialize function like this
template<typename Tp>
void is_array(Tp *x)
{

}

// The right way is 
template<typename T>
class is_pointer_impl{ static bool _() { return false; }};

template<typename Tp>
class is_pointer_imp<Tp*> { static bool _() { return true;}};

template<typename T>
bool is_pointer(T x){
    return is_pointer_impl<T>::_();
}

/*
RULE
            Type Deduction | Full Specialization | Partial Specialization
Function    Yes            | Yes                 | No                    
Class       No             | Yes                 | Yes
Alias       No             | No                  | No
Variable    No             | Yes                 | Yes

*/


/* Explicit instantiation declartion in header*/
extern template int abs(int); // or base<>(int) or abs<int>(int)
extern template class vector<int>;
extern template bool is_void_v<void>;

/*Ex: .h*/
template<typename Cs> int mblen(const char*, int);
template<> int mblen<ascii>(const char* , int);
template<> int mblen<utf8>(const char*, int);
template<typename Cs> int reverse(const char*, int);
extern template int reverse<ascii>(const char*, int);
extern template int reverse<utf8>(const char*, int);

/* .cpp*/
template<> int mblen<ascii>(const char* , int) { ... }
template<> int mblen<utf8>(const char*, int) { ... }
template<typename Cs> int reverse(const char*, int) {....}
template int reverse<ascii>(const char*, int);
template int reverse<utf8>(const char*, int);