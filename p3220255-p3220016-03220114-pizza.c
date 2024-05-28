#include "p3220255-p3220016-p3220114-pizza.h"

unsigned int seed;
int available_tel = Ntel;
int available_cook = Ncook;
int available_oven = Noven;
int available_deliverer = Ndeliverer;
int total_revenue = 0;
int total_pizzas_m = 0;
int total_pizzas_p = 0;
int total_pizzas_s = 0;
int total_success_orders = 0;
int total_failed_orders = 0;

struct timespec custOnHold;
struct timespec custOnHoldT;

pthread_mutex_t tel_mutex;
pthread_cond_t tel_cond;
pthread_mutex_t cook_mutex; 
pthread_cond_t cook_cond;
pthread_mutex_t oven_mutex;
pthread_cond_t oven_cond;
pthread_mutex_t deliverer_mutex; 
pthread_cond_t deliverer_cond; 
pthread_mutex_t revenue_mutex;
pthread_mutex_t screen_mutex;


void* pizza_order(void* arg) {
    Order* order = (Order*)arg;
    int order_id = order->id;
    int num_pizzas = order->num_pizzas;

    // Telephone
    pthread_mutex_lock(&tel_mutex);
    clock_gettime(CLOCK_REALTIME, &custOnHoldT);

    while (available_tel == 0) {
        pthread_cond_wait(&tel_cond, &tel_mutex);
    }
    available_tel--;
    pthread_mutex_unlock(&tel_mutex);
    clock_gettime(CLOCK_REALTIME, &custOnHold);

    double timeOnHold = (custOnHold.tv_sec - custOnHoldT.tv_sec);


    // Payment
    int payment_time =  rand_r(&seed) % (Tpaymenthigh - Tpaymentlow) + Tpaymentlow + 1;
    sleep(payment_time);
    int failPos = rand_r(&seed) % 100;

    if (failPos <= Pfail - 1) {
        pthread_mutex_lock(&screen_mutex);
        printf("Order %d failed.\n", order_id);
        pthread_mutex_unlock(&screen_mutex);

        pthread_mutex_lock(&tel_mutex);
        available_tel++;
        pthread_cond_signal(&tel_cond);
        pthread_mutex_unlock(&tel_mutex);

        pthread_mutex_lock(&revenue_mutex);
        total_failed_orders++;
        pthread_mutex_unlock(&revenue_mutex);

        free(order);
        return NULL;
    }

    pthread_mutex_lock(&revenue_mutex);
    for (int i = 0; i < num_pizzas; i++) {
        int pizza_type = rand_r(&seed) % 100;
        if (pizza_type < Pm ) {
            total_revenue += Cm;
            total_pizzas_m++;
        } else if (pizza_type < Pp + Pm ) {
            total_revenue += Cp;
            total_pizzas_p++;
        } else {
            total_revenue += Cs;
            total_pizzas_s++;
        }
    }
    pthread_mutex_unlock(&revenue_mutex);

    pthread_mutex_lock(&screen_mutex);
    printf("Order %d placed.\n", order_id);
    pthread_mutex_unlock(&screen_mutex);

    pthread_mutex_lock(&tel_mutex);
    available_tel++;
    pthread_cond_signal(&tel_cond);
    pthread_mutex_unlock(&tel_mutex);

    // Preparation
    pthread_mutex_lock(&cook_mutex);
    while (available_cook == 0) {
        pthread_cond_wait(&cook_cond, &cook_mutex);
    }
    available_cook--;
    pthread_mutex_unlock(&cook_mutex);

    sleep(num_pizzas * Tprep);

    pthread_mutex_lock(&cook_mutex);
    available_cook++;
    pthread_cond_signal(&cook_cond);
    pthread_mutex_unlock(&cook_mutex);

    // Oven
    pthread_mutex_lock(&oven_mutex);
    while (available_oven < num_pizzas) {
        pthread_cond_wait(&oven_cond, &oven_mutex);
    }
    available_oven -= num_pizzas;
    pthread_mutex_unlock(&oven_mutex);

    sleep(Tbake);

    pthread_mutex_lock(&oven_mutex);
    available_oven += num_pizzas;
    pthread_cond_broadcast(&oven_cond);
    pthread_mutex_unlock(&oven_mutex);

    //Packing
    pthread_mutex_lock(&deliverer_mutex);
    while (available_deliverer == 0) {
        pthread_cond_wait(&deliverer_cond, &deliverer_mutex);
    }
    available_deliverer--;
    pthread_mutex_unlock(&deliverer_mutex);

    sleep(num_pizzas * Tpack);

    
    int order_to_packaging_time = (timeOnHold + payment_time + num_pizzas * (Tprep + Tpack) + Tbake);

    pthread_mutex_lock(&screen_mutex);
    printf("The order %d got packed in %d minutes\n", order_id,order_to_packaging_time);
    pthread_mutex_unlock(&screen_mutex);

    // Delivery
    int delivery_time = rand_r(&seed) % (Tdelhigh - Tdellow) + Tdellow + 1;
    sleep(delivery_time);

    pthread_mutex_lock(&screen_mutex);
    printf("Order %d delivered in %d minutes.\n", order_id, delivery_time + payment_time + num_pizzas * (Tprep + Tprep)+ Tbake);
    pthread_mutex_unlock(&screen_mutex);

    pthread_mutex_lock(&revenue_mutex);
    total_success_orders++;
    pthread_mutex_unlock(&revenue_mutex);

    sleep(delivery_time);

    pthread_mutex_lock(&deliverer_mutex);
    available_deliverer++;
    pthread_cond_signal(&deliverer_cond);
    pthread_mutex_unlock(&deliverer_mutex);

    free(order);
    return NULL;
}
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Enter the number of orders and the seed properly\n");
        return 1;
    }

    int Ncust = atoi(argv[1]);
    unsigned int Seed = atoi(argv[2]);

    pthread_t threads[Ncust];

    for (int i = 0; i < Ncust; i++) {
        Order* order = malloc(sizeof(Order));
        order->id = i + 1;
        order->num_pizzas = rand_r(&seed) % (Norderhigh - Norderlow)+ Norderlow + 1 ;

        pthread_create(&threads[i], NULL, pizza_order, order);

        int order_time = rand_r(&seed) % (Torderhigh - Torderlow) + Torderlow + 1;
        sleep(order_time);
    }

    for (int i = 0; i < Ncust; i++) {
        pthread_join(threads[i], NULL);
    }

    int mutex_destroy = pthread_mutex_destroy(&tel_mutex);
    

    pthread_mutex_lock(&screen_mutex);
    printf("Total Revenue: %d euros\n", total_revenue);
    printf("Margarita: %d, Peperoni: %d, Special: %d\n", total_pizzas_m, total_pizzas_p, total_pizzas_s);
    printf("Completed Orders: %d, Failed Orders: %d\n", total_success_orders, total_failed_orders);
    pthread_mutex_unlock(&screen_mutex);

    return 0;
}