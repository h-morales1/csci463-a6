//***************************************************************************
//
//  Herbert Morales
//  Z1959955
//  CSCI-463-MSTR
//
//  I certify that this is my own work and where appropriate an extension
//  of the starter code provided for the assignment.
//
//***************************************************************************

#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <unistd.h>
#include <vector>
using std::cerr;

constexpr int rows = 1000; /// < the number of rows in the work matrix
constexpr int cols = 100;  /// < the number of cols in the work matrix

std::mutex stdout_lock; /// < for serializing access to stdout

std::mutex counter_lock;     /// < for dynamic balancing only
volatile int counter = rows; /// < for dynamic balancing only

std::vector<int> tcount;   /// < count of rows summed for each thread
std::vector<uint64_t> sum; /// < the calculated sum from each thread

int work[rows][cols]; /// < the matrix to be summed

/**
 * Print a message to stderr telling the user how to
 * properly run the program and then exiting with a
 * return code of 1.
 *
 * @brief Print a message detailing how to run program.
 *
 ********************************************************************************/
static void usage() {
  cerr << "Usage: reduce [-d] [-t threads]" << std::endl;
  cerr << "    -d use dynamic load-balancing(default is static load balancing)"
       << std::endl;
  cerr
      << "    -t specifies the number of threads to use(default is two threads)"
      << std::endl;
  exit(1);
}
/**
 * Use 0x1234 as the seed for rand(), then fill
 * the matrix with values generated from seed.
 *
 * @brief Fill the matrix with values.
 *
 ********************************************************************************/
void fill_matrix();
/**
 * Use stdout_lock to print that thread tid is starting, unlock
 * stdout_lock and begin processing the values in the matrix, using
 * the tid as the starting row and advancing by the num_threads.
 * After the work is finished, lock stdout_lock and print
 * that tid is ending, its tcount and the sum of the rows that tid
 * processsed.
 *
 * @brief static load balancing is used to get sum of matrix valsues.
 *
 * @param tid thread id.
 * @param num_threads number of threads.
 *
 ********************************************************************************/
void sum_static(int tid, int num_threads);
/**
 * Use stdout_lock to print that thread tid is starting, unlock
 * stdout_lock and then use a mutex lock to access the global counter
 * variable, in order to determine the next row to process.While the counter
 * is locked, no other tids can access it, instead those tids are processing
 * the values of the matrix, the row is now determined by the counter_copy.
 * after the work is finished, stdout_lock is used to print that the tid
 * is ending, its  tcount and the sum of the rows it processed.
 *
 * @brief dynamic load balancing is used to get sum of matrix valsues.
 *
 * @param tid thread id.
 *
 ********************************************************************************/
void sum_dynamic(int tid);
/**
 * Use thread::hardware_concurrency() to determine how many threads the user
 * can support. Call fill_matrix() to fill matrix with values. Determine if the
 * user selected a load balancing technique or a different number of threads
 *than the default. Check the number of threads the user input, if the thread
 *number the user input is greater than supported then the supported amount is
 *used, if nothing is entered then 2 is the default used, if the value input is
 * less than the supported amount then that amount is used. A message printing
 * the supported amount of threads is printed. tcount and sum vectors are
 *prepared with a number of elements matching the thread number to be used, with
 *the values set to 0. The function checks if the user chose dynamic, if so then
 * a vector of pointers to threads is created, passing sum_dynamic(i) as the
 *parameter for the constructor, i being the tid or index in the loop creating
 *all the threads. if dynamic was not selected then the default is used and
 *sum_static() is passed as a parameter, in a similar fashion to sum_dynamic(),
 *this time also passing the number of threads to be used to each constructor. A
 *loop then join the pointer to the thread at index i and then delete the
 *object. Calculate the total_work for all threads as well as the gross_sum for
 *all threads. Print that main is exiting, the total_work and the gross_sum.
 *Return.
 *
 * @brief Calculate sum of values in matrix using load balancing.
 *
 * @param argc argument count.
 * @param argv arguments.
 *
 ********************************************************************************/
int main(int argc, char **argv) {
  unsigned int threads_supported = std::thread::hardware_concurrency();
  unsigned int thread_num = 2;
  bool use_dynamic = false;
  std::vector<std::thread *> threads;
  int total_work = 0;
  uint64_t total_sum = 0;

  fill_matrix();

  int opt;
  while ((opt = getopt(argc, argv, "dt:")) != -1) {
    switch (opt) {
    case 'd': {
      // dynamic sum
      use_dynamic = true;
    } break;
    case 't': {
      // use arg as amount of threads to use
      std::istringstream iss(optarg);
      iss >> std::hex >> thread_num;
    } break;
    default:
      usage();
    }
  }

  if (thread_num > threads_supported) {
    thread_num = threads_supported; // if the thread num entered is more
                                    // than supported  then set thread num back
                                    // to default
  }

  std::cout << threads_supported << " "
            << "concurrent threads supported." << std::endl;

  // prepare other vectors
  tcount.assign(thread_num, 0);
  sum.assign(thread_num, 0);

  if (use_dynamic) {
    // use dynamic
    for (unsigned int i = 0; i < thread_num; i++) {
      threads.push_back(new std::thread(sum_dynamic, i));
    }
  } else {
    // use static
    for (unsigned int i = 0; i < thread_num; i++) {
      threads.push_back(new std::thread(sum_static, i, thread_num));
    }
  }

  // thread_num + 1 threads are running now
  for (unsigned int i = 0; i < thread_num; i++) {
    threads.at(i)->join();
    delete threads.at(i);
    total_work += tcount.at(i);
    total_sum += sum[i]; // calc gross sum
  }

  std::cout << "main() exiting, "
            << "total_work=" << total_work << " gross_sum=" << total_sum
            << std::endl;

  return 0;
}

void fill_matrix() {
  srand(0x1234); // seed for rand()

  for (int i = 0; i < rows; i++) {
    // start at row 0
    for (int j = 0; j < cols; j++) {
      // start at col 0
      work[i][j] = rand(); // fill matrix
    }
  }
}

void sum_static(int tid, int num_threads) {
  // use lock for std
  stdout_lock.lock();
  std::cout << "Thread " << tid << " starting" << std::endl;
  stdout_lock.unlock();

  // do work starting at row determined by tid
  for (int i = tid; i < rows; i += num_threads) {
    // start at row tid+num_threads
    tcount[tid] += 1; // increment row count for this tid
    for (int j = 0; j < cols; j++) {
      // start at col 0
      sum[tid] += work[i][j]; // sums for each tid
    }
  }

  // work finished
  stdout_lock.lock();
  std::cout << "Thread " << tid << " ending "
            << "tcount=" << tcount[tid] << " sum=" << sum[tid] << std::endl;
  stdout_lock.unlock();
}

void sum_dynamic(int tid) {
  // lock for std
  stdout_lock.lock();
  std::cout << "Thread " << tid << " starting" << std::endl;
  stdout_lock.unlock();

  bool finished = false;
  while (!finished) {
    int count_copy;

    counter_lock.lock(); // so no other tids can touch value
    {
      if (counter > 0)
        --counter;
      else
        finished = true;
      count_copy = counter; // update the counter copy
    }
    counter_lock.unlock();

    if (!finished) {
      // work can be done using value of count copy
      (void)count_copy; // remove compiler warning
      ++tcount[tid];

      // do work starting at row determined by tid
      for (int j = 0; j < cols; j++) {
        // start at col 0 and row count_copy
        sum[tid] += work[count_copy][j];
      }
    }
  }

  // work finished
  stdout_lock.lock();
  std::cout << "Thread " << tid << " ending "
            << "tcount=" << tcount[tid] << " sum=" << sum[tid] << std::endl;
  stdout_lock.unlock();
}
