#include <soci/soci.h>
#include <iostream>
#include <soci/mysql/soci-mysql.h>
#include <vector>
#include <chrono> 

struct MyClass
{
    std::string name;
    int age;
};

namespace soci
{
    template<> 
    struct type_conversion<MyClass>
    {
        typedef values base_type;

        static void from_base(const values& v, indicator /* ind */, MyClass& event)
        {
            event.name = v.get<std::string>("name");
            event.age = v.get<int>("age");
        }

        static void to_base(const MyClass& event, values& v, indicator& ind)
        {
            v.set("name", event.name);
            v.set("age", event.age);
            ind = i_ok;
        }
    };
}


std::vector<MyClass> getData(const std::string& my_st)
{
    constexpr int SIZE = 1000;
    std::vector<MyClass> myClasses;
    myClasses.reserve(SIZE);

    for (int i =0; i < SIZE; i++)
    {
        myClasses.push_back(MyClass{my_st + std::to_string(i), i});
    }

    return myClasses;
}


int main()
{
    //char const* const connectString = "db=lu_test user=stanly password='rjs047' host=localhost unix_socket=/var/run/mysqld/mysqld.sock";
    char const* const connectString = "db=lu_test user=stanly password='zoo9Ieho!1qaz' host=127.0.0.1 port=3306";
    std::vector<MyClass> data1 = getData("a");
    std::vector<MyClass> data2 = getData("b");
    std::vector<MyClass> data3 = getData("c");

    try {
        soci::session sql(soci::mysql, connectString);
        sql << "CREATE TABLE IF NOT EXISTS MyClass ("
            "name VARCHAR(255),"
            "age INT)";

        sql << "TRUNCATE TABLE MyClass";
        sql.commit();

        /////////////////////////////////////////////////////////////////////////////////////////
        soci::transaction tr2(sql);
        auto start = std::chrono::high_resolution_clock::now(); 
        for (const MyClass& instance : data2) 
        {
            sql << "INSERT INTO MyClass (name, age) VALUES (:name, :age)",
                soci::use(instance);
        }
        tr2.commit();
        auto end = std::chrono::high_resolution_clock::now(); 
        std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Insertion by instance binding: " << duration.count() << " milliseconds" << std::endl;

        /////////////////////////////////////////////////////////////////////////////////////////
        start = std::chrono::high_resolution_clock::now(); 
        soci::transaction tr1(sql);
        for (const MyClass& instance : data1) 
        {
            sql << "INSERT INTO MyClass (name, age) VALUES (:name, :age)",
                soci::use(instance.name), soci::use(instance.age);
        }
        tr1.commit();
        end = std::chrono::high_resolution_clock::now(); 

        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Insertion by instance: " << duration.count() << " milliseconds" << std::endl;
        

        /////////////////////////////////////////////////////////////////////////////////////////
        
        start = std::chrono::high_resolution_clock::now(); 
        soci::transaction tr(sql);
        soci::statement stmt(sql);
        stmt.alloc();
        stmt.prepare("INSERT INTO MyClass (name, age) VALUES (:name, :age)");

        for (const MyClass& instance : data3) 
        {
            // Bind values to the prepared statement in bulk
            stmt.exchange(soci::use(instance.name, "name"));
            stmt.exchange(soci::use(instance.age, "age"));
            //stmt.exchange(soci::use(instance));
            stmt.define_and_bind();
            stmt.execute(true);
        }
        tr.commit();
        end = std::chrono::high_resolution_clock::now(); 
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Insertion by bulk: " << duration.count() << " milliseconds" << std::endl;

        /////////////////////////////////////////////////////////////////////////////////////////
        start = std::chrono::high_resolution_clock::now(); 
        long long i = 0;
        soci::rowset<MyClass> rs_p = (sql.prepare << "select * from MyClass");
        for (soci::rowset<MyClass>::const_iterator it = rs_p.begin(); it != rs_p.end(); ++it)
        {
            i += it->age;
        }

        end = std::chrono::high_resolution_clock::now(); 
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Read by bulk: " << duration.count() << " milliseconds " << i << std::endl;


        /////////////////////////////////////////////////////////////////////////////////////////
        i = 0;
        const int BATCH_SIZE = 3000;
        std::vector<int> valsOut(BATCH_SIZE);
        start = std::chrono::high_resolution_clock::now();
        
        soci::statement st = (sql.prepare <<
                        "select age from MyClass",
                        soci::into(valsOut));
        
        st.execute();
        while (st.fetch())
        {
            std::vector<int>::iterator pos;
            for(pos = valsOut.begin(); pos != valsOut.end(); ++pos)
            {
                i += *pos;
            }

            valsOut.resize(BATCH_SIZE);
        }

        end = std::chrono::high_resolution_clock::now(); 
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Read by bulk2: " << duration.count() << " milliseconds " << i << std::endl;
    } 
    catch (const soci::soci_error &e) 
    {
        std::cout << "Error creating table: " << e.what() << std::endl;
    }
}