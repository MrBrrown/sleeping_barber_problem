#include <iostream>
#include <queue>
#include <thread>
#include <random>
#include <chrono>

std::mutex lock_queue_mutex;
std::mutex lock_print_mutex;
std::mutex lock_barber_mutex;
std::condition_variable barber_alarm;
bool is_barber_sleep = true;

using namespace std;

[[noreturn]] void customerBehavior(queue<int> &customersQueue, int places)
{
    int customer_id = 1;

    random_device rd;       // Гениратор случайных числе
    mt19937 gen(rd());
    uniform_int_distribution<> dist(4,8);       //Диапозон прихода клиентов в секундах

    while (true)
    {
        this_thread::sleep_for(chrono::seconds{dist(gen)});        // Ожидание Клиента
        {
            unique_lock<mutex> locker(lock_print_mutex);        // Оповещение о прибыти клиента
            cout << "Client " << customer_id << " is came!" << endl;
        }

        if (customersQueue.size() == places)        // В парикмахерской нет свободных мест
        {
            {
                unique_lock<mutex> locker(lock_print_mutex);        // Оповещение об уходе клиента
                cout << "Client " << customer_id << " is leave! All places are occupied" << endl;
            }
            customer_id++;
            continue;
        }

        {
            unique_lock<mutex> locker(lock_queue_mutex);        // Добавление клиента в очередь
            customersQueue.push(customer_id);
        }

        if (is_barber_sleep)
        {
            is_barber_sleep = false;
            barber_alarm.notify_one();
        }

        customer_id++;
    }
}

[[noreturn]] void barberBehavior(queue<int> &customersQueue)
{
    random_device rd;       // Гениратор случайных числе
    mt19937 gen(rd());
    uniform_int_distribution<> dist(5,10);       //Диапозон стрижки клиентов в секундах
    int cutting_customer_id = 0;

    while (true)
    {
        if (is_barber_sleep)
        {
            {
                unique_lock<mutex> locker(lock_print_mutex);
                cout << "Barber is sleeping!" << endl;
            }
            {
                unique_lock<mutex> locker(lock_barber_mutex);   // Ожидание клиентов
                barber_alarm.wait(locker);
            }
            {
                unique_lock<mutex> locker(lock_print_mutex);
                cout << "Barber woke up!" << endl;
            }
        }

        {
            unique_lock<mutex> locker(lock_queue_mutex);        // "Клиент заходит в кабинет"
            cutting_customer_id = customersQueue.front();
            customersQueue.pop();
        }

        {
            unique_lock<mutex> locker(lock_print_mutex);
            cout << "Client " << cutting_customer_id << " getting a haircut"<<endl;
        }
        this_thread::sleep_for(chrono::seconds{dist(gen)});        // Процесс стрижки
        {
            unique_lock<mutex> locker(lock_print_mutex);
            cout << "Client " << cutting_customer_id << " haircut done."<<endl
                 <<"Client leave, in queue "<<customersQueue.size()<<" clients"<<endl;
        }

        if (customersQueue.empty())
            is_barber_sleep = true;
    }
}

int main()
{
    queue<int> customers_queue;
    int places;
    cout<<"Enter the number of places in the barbershop: ";
    cin>>places;

    thread barber(barberBehavior, ref(customers_queue));
    thread customers(customerBehavior, ref(customers_queue), places);

    customers.join();
    barber.join();
}