#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>

#define NO_OF_PRODUCTS 6
#define NO_OF_CUSTOMERS 10
#define NO_OF_THREADS 5

struct Product {
  int product_id;
  int price;
  int quantity_in_stock;
};

struct Customer {
  int customer_id;
  int balance;
  int ordered_items[99][2];
  int purchased_items[99][2];
  int ordered_items_size;
  int purchased_items_size;
};

typedef struct {
  pthread_mutex_t product_locks[NO_OF_PRODUCTS];
  struct Product products[NO_OF_PRODUCTS];
  struct Customer customers[NO_OF_CUSTOMERS];
} shared_data_t;


typedef struct {
  int customer_id;
  int product_id;;
  int product_quantity;
  int direct;
} ThreadArgs;

typedef struct {

  int customer;
  ThreadArgs* orders;
  int size;
} ArrayArgs;

void* order_product(shared_data_t *shared_data, void* arg) {

  ThreadArgs* args = (ThreadArgs*) arg;
  ThreadArgs myargs = *args;
  int customer_id = myargs.customer_id + 1;
  int ordered_quantity = myargs.product_quantity;
  int product_id = myargs.product_id +1;
  int direct = myargs.direct;

  pthread_mutex_lock(&(shared_data->product_locks[product_id-1]));

  shared_data->customers[customer_id-1].ordered_items[shared_data->customers[customer_id-1].ordered_items_size][0] = (int) product_id;
  shared_data->customers[customer_id-1].ordered_items[shared_data->customers[customer_id-1].ordered_items_size][1] = (int) ordered_quantity;
  shared_data->customers[customer_id-1].ordered_items_size++;


  printf("Customer %d: -Balance: %d, -Quantity Ordered: %d\nProduct %d: -Price: %d, -Quantity in stock: %d\n", customer_id, shared_data->customers[customer_id-1].balance, ordered_quantity, product_id, shared_data->products[product_id-1].price, shared_data->products[product_id-1].quantity_in_stock);

  if(shared_data->products[product_id-1].quantity_in_stock < ordered_quantity) {
    printf("Customer %d RESULT: Customer %d was unable to purchase Product %d because there isn't enough stock\n",customer_id, customer_id, product_id);
  }

  else if(shared_data->customers[customer_id-1].balance < shared_data->products[product_id-1].price*ordered_quantity) {
    printf("Customer %d RESULT: Customer %d was unable to purchase Product %d because of insufficient balance\n",customer_id, customer_id, product_id);
  }

  else {


  printf("Customer %d RESULT: Customer %d purchased %d of Product %d. Previous Quantity: %d. New Quantity: %d \n", customer_id ,customer_id,ordered_quantity, product_id, shared_data->products[product_id-1].quantity_in_stock, shared_data->products[product_id-1].quantity_in_stock-ordered_quantity);
  shared_data->products[product_id-1].quantity_in_stock -= ordered_quantity;

  shared_data->customers[customer_id-1].purchased_items[shared_data->customers[customer_id-1].purchased_items_size][0] = product_id;
  shared_data->customers[customer_id-1].purchased_items[shared_data->customers[customer_id-1].purchased_items_size][1] = ordered_quantity;
  shared_data->customers[customer_id-1].purchased_items_size++;

  sleep(3);

  }
  pthread_mutex_unlock(&(shared_data->product_locks[product_id-1]));

  if(direct) free(arg);
  return NULL;
}

void* order_products(int shmid, void *arg) {

  shared_data_t *shared_data = (shared_data_t *) shmat(shmid, NULL, 0);

  ArrayArgs* args = (ArrayArgs*) arg;
  ArrayArgs myArgs = *args;

  ThreadArgs* orders= myArgs.orders;
  int size = myArgs.size;
  int customer_id = myArgs.customer + 1;

  printf("Customer %d ordered %d orders\n", customer_id, size);

  for(int i = 0; i<size; i++) {

    order_product(shared_data, (void*) &orders[i]);

  }


  return NULL;

}

int main(int argc, char const *argv[]) {

  srand(time(0));

  int shmid;
  shared_data_t *shared_data;


  shmid = shmget(IPC_PRIVATE, sizeof(shared_data_t), IPC_CREAT | 0666);
  if(shmid == -1){
    perror("shmget error");
    exit(1);
  }


  shared_data = (shared_data_t *) shmat(shmid, NULL, 0);

  if(shared_data == (shared_data_t *) -1){
    perror("shmat error");
    exit(1);
  }



  pthread_mutexattr_t mutex_attr;
  pthread_mutexattr_init(&mutex_attr);
  pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);


  for(int i = 0; i<NO_OF_PRODUCTS; i++) {

    printf("Product %d\n", i+1);
    shared_data->products[i].product_id = i+1;
    shared_data->products[i].price = (rand() % 200) + 1;
    shared_data->products[i].quantity_in_stock = (rand() %10) + 1;

    pthread_mutex_init(&(shared_data->product_locks[i]), &mutex_attr);
  }

  for(int i = 0; i<NO_OF_CUSTOMERS; i++) {
    printf("Customer %d\n", i+1);

    shared_data->customers[i].customer_id = i+1;
    shared_data->customers[i].balance = (rand() % 200) + 1;

  }

  for(int i = 0; i<NO_OF_CUSTOMERS; i++) {

    int pid = fork();

    if(pid < 0) {
      perror(" fork error ");
      exit(1);
    }

    else if(pid == 0){ // Child Process

      printf("child process\n");
      int no_of_orders = rand()%5 + 1;

      ThreadArgs* orders = (ThreadArgs*) malloc(no_of_orders*sizeof(ThreadArgs));

      int customer = i;

      for(int j = 0; j<no_of_orders; i++) {

        orders[i].customer_id = customer;
        orders[i].product_id = (rand() % NO_OF_PRODUCTS);
        orders[i].product_quantity = (rand() % 4 + 1);
        orders[i].direct = 0;

      }

      ArrayArgs* array_args = (ArrayArgs*) malloc(sizeof(ArrayArgs));

      array_args->customer = customer;
      array_args->orders = orders;
      array_args->size =  no_of_orders;

      order_products(shmid, array_args);

      break;

    } else continue; // parent process

  }




  if (shmctl(shmid, IPC_RMID, NULL) == -1) {
    perror("shmctl error");
    return 1;
  }

  return 0;
}
