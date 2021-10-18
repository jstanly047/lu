/*
SC Acq/Rel

↓↓↓ Acquire ↓↓↓  // non atomic code can be reordered below

↑↑↑ Release ↑↑↑ // non atomic code can be reordered above


*/

std::mutex cMutex;
std::mutex dMutex;

// Thread 1
{
    lock_guard<mutex> lock(cMutex);
    c =1;
}

// Thread 2
{
    lock_guard<mutex> lock(dMutex);
    d =1;
}

/*
 standard allow above; doesn't need worry about c & d in same cache line
*/

struct { char c; char d;}

// Thread 1
{
    lock_guard<mutex> lock(cMutex);
    s.c =1;
}

// Thread 2
{
    lock_guard<mutex> lock(dMutex);
    s.d =1;
}

/*
 standard allow above; doesn't need worry about c & d in same cache line, padding wil added if nessary
*/



if (cond)
 lock x;

if (cond)
  use x

if (cond)
  unlock x

/*
Above is and anti pattern avoid if possible

Let say compiler generated code like for second if condition (due to compier bug)
r1 = x;
x = 42;  // write to lock without lock can happen here
if (!cond)  
    x = r1; // write to lock without lock cond since cond  false

*/

void f()
{
    if (doOptionalWork) xMutex.lock();

    for (..)
        if (doOptionalWork) ++x;

    if (doOptionalWork) xMutex.unlock();
}

/* again above loop can be optimized like 

r1 = x;
for (...)
    if (doOptionalWork) ++r1;

x = r1; // write to x without lock can happend here

*/




// Relaxed memeor order use case

//CASE 1
std::atomic<int> count;

int main()
{
    launch_workers();
    ..
    ..
    join_workers();
    cout << count.load(memory_order_relaxed) << endl;
}

//worker thread 1..N
while(..){
    ...
    if (...)
        count.featch_add(1, memory_order_relaxed);
}


//CASE2
std::atomic<bool> stop;
std::atomic<bool> dirty;

int main()
{
    launch_workers();
    ..
    stop=true; // SC
    ..
    join_workers();

    if (dirty.load(memory_order_acquire))
        clean_up_dirty_stuff();
}

//Worker 1...N

while(!stop.load(memory_order_relaxed))
{
    if (..)
        dirty.store(true, memory_order_released);
    ..
    ..
};

/* stop can be relaxed since as per the program execution of thread can be any number of time
After dirty flag is check we are doing clean up on work done by threads....
so we need to put acquire so clean is not reordered up  
*/


// CASE 3

//Thread a
control_block_ptr = other->control_block_ptr;
control_block_ptr->ref.fetch_add(1, memeory_order_relaxed);



// Thread b
if (control_block_ptr->ref.fetch_sub(1, memory_order_acq_rel) == 0)
{
    delete control_block_ptr;
}


/*
 reference count increment is just an operation can be relxed since we dont do anythin based on reference count on increment
On the other hand on decrement we based on count destory the object. and no reorder can happen in this block
if you put release the delete control_bock_ptr my reorder mean executed before check
*/

// Double check

automic<widget*> widget::instance = nullptr;
automic<bool> widget::create=false;

widget* widget::get_instance()
{
    if (instance == nullptr)
    {
        if (!create.exchange(true))
        {
            instance = new widget();
        }
        else while(instance == nullptr) {}
    }
    return instance;
}





