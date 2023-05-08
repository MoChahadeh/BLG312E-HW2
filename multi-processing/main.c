#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
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

void order_product(shared_data_t *shared_data, ThreadArgs arg) {

  ThreadArgs myargs = arg;
  int customer_id = myargs.customer_id + 1;
  int ordered_quantity = myargs.product_quantity;
  int product_id = myargs.product_id +1;
  int direct = myargs.direct;

  printf("\nCustomer %d is waiting to lock Product %d\n", customer_id, product_id);
  pthread_mutex_lock(&(shared_data->product_locks[product_id-1]));
  printf("\nCustomer %d Acquired a lock on product %d\n", customer_id, product_id);

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

  sleep(6);

  }
  pthread_mutex_unlock(&(shared_data->product_locks[product_id-1]));
  printf("\nCustomer %d released lock on product %d\n", customer_id, product_id);

}

void order_products(int shmid, ArrayArgs *args) {

  shared_data_t *shared_data_fork = (shared_data_t *) shmat(shmid, NULL, 0);

  if(shared_data_fork == (shared_data_t *) -1) {
    perror("child shmat error");
    exit(1);
  }

  ArrayArgs myArgs = *args;
  ThreadArgs* orders= myArgs.orders;
  int size = myArgs.size;
  int customer_id = myArgs.customer + 1;

  printf("Customer %d ordered %d orders\n", customer_id, size);

  for(int i = 0; i<size; i++) {

    order_product(shared_data_fork, orders[i]);

  }

  printf("Customer %d finished ordering\n", customer_id);


}

int main(int argc, char const *argv[]) {

  srand(time(NULL));

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

    shared_data->products[i].product_id = i+1;
    shared_data->products[i].price = (rand() % 200) + 1;
    shared_data->products[i].quantity_in_stock = (rand() %10) + 1;

    printf("Product %d Price %d Quantity %d  \n", i+1, shared_data->products[i].price, shared_data->products[i].quantity_in_stock);


    pthread_mutex_init(&(shared_data->product_locks[i]), &mutex_attr);
  }

  for(int i = 0; i<NO_OF_CUSTOMERS; i++) {

    shared_data->customers[i].customer_id = i+1;
    shared_data->customers[i].balance = (rand() % 200) + 1;

    printf("Customer %d with balance %d\n", i+1, shared_data->customers[i].balance);

  }

  int pid;

  ArrayArgs* all_args = malloc(NO_OF_CUSTOMERS*sizeof(ArrayArgs));

  for(int i = 0; i<NO_OF_CUSTOMERS; i++) {


    pid = fork();

    if(pid < 0) {
      perror(" fork error ");
      exit(1);
    }

    else if(pid == 0){ // Child Process

      srand(time(NULL) ^ (getpid()<<16));
      int no_of_orders = (rand()%5) + 1;
      printf("\nthis is a random number: %d\n", rand() % 10);

      ThreadArgs* orders = (ThreadArgs*) malloc(no_of_orders*sizeof(ThreadArgs));

      int customer = i;

      for(int j = 0; j<no_of_orders; j++) {

        orders[j].customer_id = customer;
        orders[j].product_id = (rand() % NO_OF_PRODUCTS);
        orders[j].product_quantity = (rand() % 5) + 1;
        orders[j].direct = 0;

      }

      all_args[i].customer = customer;
      all_args[i].orders = orders;
      all_args[i].size =  no_of_orders;

      order_products(shmid, (void *) &all_args[i]);

      exit(0);

    } else continue; // parent process

  }


  for(int i = 0 ; i<NO_OF_CUSTOMERS; i++) {
    wait(NULL);
  }

  printf("\nPress any key to show summaries of customers...\n");
  getchar();

  printf("\nSUMMARIES:\n");
  sleep(1);

  for (int i = 0; i<NO_OF_CUSTOMERS; i++) {

    printf("\n----------------\n\nCustomer %d Summary:\n", i+1);
    sleep(1);

    printf("\nOrdered Products:\n\n");
    printf("%-15s %-15s \n", "Product ID", "Quantity");

    for(int j = 0; j<shared_data->customers[i].ordered_items_size;j++){

      int product_id = shared_data->customers[i].ordered_items[j][0];
      int quantity =   shared_data->customers[i].ordered_items[j][1];

      printf("%-15d %-15d \n",  product_id, quantity);

    }

    sleep(2);

    printf("\nPurchased Products:\n\n");
    printf("%-15s %-15s \n", "Product ID", "Quantity");

    for(int j = 0; j<shared_data->customers[i].purchased_items_size; j++) {

      int product_id = shared_data->customers[i].purchased_items[j][0];
      int quantity   = shared_data->customers[i].purchased_items[j][1];

      printf("%-15d %-15d \n",  product_id,  quantity);

    }

    sleep(4);

  }

  if(shmctl(shmid,IPC_RMID, NULL) == -1) {

    perror("SHMCTL ERROR");
    exit(1);

  }

  printf("\nmemory destroyed\n");

  printf("\n\n PROGRAM END \n\n");


  return 0;
}
