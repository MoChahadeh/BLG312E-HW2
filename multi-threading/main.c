#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#define NO_OF_PRODUCTS 5
#define NO_OF_CUSTOMERS 3

pthread_mutex_t product_locks[NO_OF_PRODUCTS];

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


struct Product products[NO_OF_PRODUCTS];
struct Customer customers[NO_OF_CUSTOMERS];


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

void* order_product(void* arg) {

  ThreadArgs* args = (ThreadArgs*) arg;
  ThreadArgs myargs = *args;
  int customer_id = myargs.customer_id + 1;
  int ordered_quantity = myargs.product_quantity;
  int product_id = myargs.product_id +1;
  int direct = myargs.direct;

  printf("Customer %d waiting to lock product %d...\n",customer_id, product_id);
  pthread_mutex_lock(&product_locks[product_id-1]);

  printf("Customer %d locked product %d\n", customer_id, product_id);
  customers[customer_id-1].ordered_items[customers[customer_id-1].ordered_items_size][0] = (int) product_id;
  customers[customer_id-1].ordered_items[customers[customer_id-1].ordered_items_size][1] = (int) ordered_quantity;
  customers[customer_id-1].ordered_items_size++;



  if(products[product_id-1].quantity_in_stock < ordered_quantity) {
    printf("Customer%d(%d, %d): Fail! Not enough stock. \n", customer_id, product_id, ordered_quantity);
  }

  else if(customers[customer_id-1].balance < products[product_id-1].price*ordered_quantity) {
    printf("Customer%d(%d, %d): Fail! Insufficient Balance. \n", customer_id, product_id, ordered_quantity);
  }

  else {


    printf("Customer%d(%d, %d): Success! Previous Quantity: %d. New Quantity: %d \n", customer_id , product_id, ordered_quantity, products[product_id-1].quantity_in_stock, products[product_id-1].quantity_in_stock-ordered_quantity);

    products[product_id-1].quantity_in_stock -= ordered_quantity;

    customers[customer_id-1].balance -= products[product_id-1].price*ordered_quantity;
    customers[customer_id-1].purchased_items[customers[customer_id-1].purchased_items_size][0] = product_id;
    customers[customer_id-1].purchased_items[customers[customer_id-1].purchased_items_size][1] = ordered_quantity;
    customers[customer_id-1].purchased_items_size++;

    fflush(stdout);
    sleep(3);

  }
  pthread_mutex_unlock(&product_locks[product_id-1]);
  printf("Customer %d released product %d\n", customer_id, product_id);

  if(direct) free(arg);
  return NULL;
}

void* order_products(void* arg) {

  ArrayArgs* args = (ArrayArgs*) arg;
  ArrayArgs myArgs = *args;

  ThreadArgs* orders= myArgs.orders;
  int size = myArgs.size;
  int customer_id = myArgs.customer + 1;

  printf("Customer %d ordered %d orders\n", customer_id, size);

  pthread_t my_subthreads[size];

  for(int i = 0; i<size; i++) {

    int err = pthread_create(&my_subthreads[i],NULL,order_product,(void*) &orders[i]);

    if(err) {
      perror("subthreads create error");
      exit(1);
    }

  }

  for(int i = 0; i<size; i++) {
    int err = pthread_join(my_subthreads[i], NULL);

    if(err) {
      perror("subthreads join error");
      exit(1);
    }
  }

  return NULL;

}

void print_summaries() {
  printf("\nSUMMARIES:\n");
  sleep(1);

  for (int i = 0; i<NO_OF_CUSTOMERS; i++) {

    printf("\n----------------\n\nCustomer %d Summary:\n", i+1);
    sleep(1);

    printf("\nOrdered Products:\n\n");
    printf("%-15s %-15s \n", "Product ID", "Quantity");

    for(int j = 0; j<customers[i].ordered_items_size;j++){

      int product_id = customers[i].ordered_items[j][0];
      int quantity =   customers[i].ordered_items[j][1];

      printf("%-15d %-15d \n",  product_id, quantity);

    }

    sleep(2);

    printf("\nPurchased Products:\n\n");
    printf("%-15s %-15s \n", "Product ID", "Quantity");

    for(int j = 0; j<customers[i].purchased_items_size; j++) {

      int product_id = customers[i].purchased_items[j][0];
      int quantity =   customers[i].purchased_items[j][1];

      printf("%-15d %-15d \n",  product_id,  quantity);

    }

    sleep(1);
    printf("\nFinal Balance: $%d\n", customers[i].balance);

    sleep(4);

  }

  printf("\n----------------------------\nPRODUCTS:\n");

  sleep(1);
  for(int i = 0 ;i<NO_OF_PRODUCTS; i++) {

    printf("\nProduct %d Final Quantity: %d\n", i+1 ,products[i].quantity_in_stock);

    sleep(1);
  }

}

int main(int argc, char const *argv[]) {

  srand(time(NULL));


  printf("\n--------------------\n\n");
  for (int i = 0; i < NO_OF_PRODUCTS; i++) {


    products[i].product_id = i+1;
    products[i].price = (rand() %200) + 1;
    products[i].quantity_in_stock = (rand() % 10) + 1;

    printf("Product Id: %d, Product Price: $%d, Product Quantity: %d \n", products[i].product_id, products[i].price, products[i].quantity_in_stock);

    pthread_mutex_init(&product_locks[i], NULL);

  }

  printf("\n\n--------------------\n\n");
  sleep(2);

  for(int i = 0; i< NO_OF_CUSTOMERS; i++) {

    customers[i].customer_id = i+1;
    customers[i].balance = (rand() %500) +1;

    printf("Customer %d, Balance: $%d\n", customers[i].customer_id, customers[i].balance);

  }

  printf("\n\n--------------------\n\n");
  sleep(2);

  pthread_t my_threads[NO_OF_CUSTOMERS];

  for(int i = 0; i< NO_OF_CUSTOMERS; i++) {

    int no_of_orders = rand()%4 + 1;

    ThreadArgs* orders = (ThreadArgs*) malloc(no_of_orders*sizeof(ThreadArgs));

    int customer = i;

    for(int i = 0; i<no_of_orders; i++) { // for loop for creating individual orders

      orders[i].customer_id = customer;
      orders[i].product_id = (rand() % NO_OF_PRODUCTS);
      orders[i].product_quantity = (rand() % 4 + 1);
      orders[i].direct = 0;

    }

    ArrayArgs* array_args = (ArrayArgs*) malloc(sizeof(ArrayArgs)); // parameter that holds all the orders created

    array_args->customer = customer;
    array_args->orders = orders;
    array_args->size =  no_of_orders;

    int err1 = pthread_create(&my_threads[i], NULL, order_products, (void *) array_args); // thread creation

    if(err1) {
      perror("thread create error\n");
      exit(1);
    }


  }

  // joining threads so that the main thread waits for all threads to finish before continuing.
  for(int i = 0; i<NO_OF_CUSTOMERS; i++) {

    int rc = pthread_join(my_threads[i], NULL);

    if(rc != 0) {
      perror("Pthread join error");
      exit(1);
    }
  }


  printf("\nPress Return to show summaries of customers...\n");
  getchar();

  print_summaries(); //function to print summary of customers and products

  printf("\n\n PROGRAM END \n\n");


  pthread_exit(NULL);

  return 0;
}
