#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define NO_OF_PRODUCTS 10
#define NO_OF_CUSTOMERS 10
#define NO_OF_THREADS 3

pthread_mutex_t product_locks[NO_OF_PRODUCTS];

struct Product {
  int product_id;
  int price;
  int quantity_in_stock;
};

struct Customer {
  int customer_id;
  int balance;
  int ordered_items[10][2];
  int purchased_items[10][2];
};


struct Product products[NO_OF_PRODUCTS];
struct Customer customers[NO_OF_CUSTOMERS];


typedef struct {
  int customer_id;
  int product_id;;
  int product_quantity;
} ThreadArgs;


void* thread_function(void* arg) {

  ThreadArgs* args = (ThreadArgs*) arg;
  ThreadArgs myargs = *args;
  int customer_id = myargs.customer_id + 1;
  int ordered_quantity = myargs.product_quantity;
  int product_id = myargs.product_id +1;

  if(products[product_id-1].quantity_in_stock < ordered_quantity) {
    printf("Customer %d was unable to purchase Product %d because there isn't enough stock\n", customer_id, product_id);
    return NULL;
  }

  if(customers[customer_id-1].balance < products[product_id-1].price) {
    printf("Customer %d was unable to purchase Product %d because of insufficient balance\n", customer_id, product_id);
  }

  pthread_mutex_lock(&product_locks[product_id-1]);

  printf("Customer %d purchased %d of Product %d. Previous Quantity: %d", customer_id, product_id, products[product_id-1].quantity_in_stock);
  products[product_id].quantity_in_stock -= ordered_quantity;
  printf(", New Quantity: %d.\n", products[product_id-1].quantity_in_stock);
  sleep(1);
  pthread_mutex_unlock(&product_locks[product_id-1]);


  return NULL;
}

int main(int argc, char const *argv[]) {

  srand(time(0));
  for (int i = 0; i < NO_OF_PRODUCTS; i++) {


    products[i].product_id = i+1;
    products[i].price = (rand() %200) + 1;
    products[i].quantity_in_stock = (rand() % 10) + 1;

    printf("Product Id: %d, Product Price: %d, Product Quantity: %d \n", products[i].product_id, products[i].price, products[i].quantity_in_stock);

    pthread_mutex_init(&product_locks[i], NULL);

  }

  for (int i = 0; i < NO_OF_CUSTOMERS; i++) {

    customers[i].customer_id = i+1;
    customers[i].balance = (rand() % 200) + 1;

  }

  pthread_t mythreads[NO_OF_THREADS];

  for(int i = 0; i < NO_OF_THREADS; i++) {

    int ordered_product = (rand() % NO_OF_PRODUCTS);
    int ordered_quantity = (rand() % 10) + 1;
    int customer = (rand() % NO_OF_CUSTOMERS);

    printf("%d %d %d\n", customer+1, ordered_quantity, ordered_product+1);

    ThreadArgs* myargs = malloc(sizeof(ThreadArgs));

    myargs->customer_id = customer;
    myargs->product_id = ordered_product;
    myargs->product_quantity = ordered_quantity;

    int rc = pthread_create(&mythreads[i], NULL, thread_function, (void*) myargs);
    if(rc != 0) {
      perror("Pthread create");
      exit(1);
    }

  }

  for(int i = 0; i<NO_OF_THREADS; i++) {

    int rc = pthread_join(mythreads[i], NULL);

    if(rc != 0) {
      perror("Pthread join error");
      exit(1);
    }
  }

  pthread_exit(NULL);

  return 0;
}
