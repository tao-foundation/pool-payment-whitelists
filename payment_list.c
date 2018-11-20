/* License terms::
                     GNU GENERAL PUBLIC LICENSE
                       Version 3, 29 June 2007

                       refer : LICENSE

                    copyright 2016 (c) trustfarm.io

                    Pool and Blockchain and IoT Platform provider

                    Contacts Email::

                    cpplover@trustfarm.net, neinnil@trustfarm.net
                    trustfarm.info@gmail.com
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <hiredis.h>
#include <getopt.h>

const char *g_whitelist[] = { 
    "0x068ac88c5a50f574xxxxxxxxxxxxxxxxx1e8784c",
    "0xbefxxxxxxxxxxxxxxxxcbd99ffc287999b09b669",
    "0x8df9f63bddd6918590fd283969cf9f3bd14b1793",
    NULL
};

typedef struct miner_balance {
    int64_t     balance; 
    int64_t     immature; 
    int64_t     bakbalance; 
    int64_t     bakimmature; 
    int64_t     paid; 
} MinerBalance;


int print_whitelist(char **ppwhitelist)
{
    int index;

    printf("whitelisted address\n");
	for (index = 0 ; ppwhitelist[index]!=NULL; index++)
	{
		printf("whitelist #%d : %s\n", index, ppwhitelist[index]);
	} 
    return index;
}

int check_whitelisted(char *minerkey, char **ppwhitelist)
{
    int index, found = 0;
    char miner[512];

    strcpy(miner,&minerkey[11]);    // eth:miners

    printf("miners'key:%s' [%s] address\n", minerkey, miner);
	for (index = 0 ; ppwhitelist[index]!=NULL; index++)
	{
        int cmp,found = -1;
		cmp = strncasecmp(miner, ppwhitelist[index], 10 );
        // printf("whitelist #%d : %s\n", index, ppwhitelist[index]);
        if (cmp == 0)
        {
            found = 1;
            printf("\n\n Found whitelist \n\n");
            return found;
        }
	}
    return found;
}

int moverminerkeybalance(MinerBalance *minerval, int64_t *int64array)
{
    minerval->balance = int64array[0];
    minerval->immature = int64array[1];
    minerval->bakbalance = int64array[2];
    minerval->bakimmature = int64array[3];
    minerval->paid = int64array[4];

    minerval->bakbalance += minerval->balance;
    minerval->bakimmature += minerval->immature;
    minerval->balance = 0;
    minerval->immature = 0;
    minerval->paid = 0;

    return 1;
}

int copyMinerKey(MinerBalance *minerval, int64_t *int64array)
{
    minerval->balance = int64array[0];
    minerval->immature = int64array[1];
    minerval->bakbalance = int64array[2];
    minerval->bakimmature = int64array[3];
    minerval->paid = int64array[4];

    return 1;
}


int64_t return_node_balance( char *mineraccount )
{
   FILE *in=NULL;
   char temp[256];
   int64_t balance;
         
   /* sameple command
    * char *gethcmd = "./geth attach http://localhost:8545 --exec \"var value=web3.eth.getBalance('0x75b9e06d526d7a4a5768635f924383f6ad82ad81'); web3.fromWei(value, 'gwei');\" ";
    */
   char *gethcmd = "./geth attach http://localhost:8545 --exec \"var value=web3.eth.getBalance('%s'); web3.fromWei(value, 'gwei');\" ";
   char cmd[2048];

   sprintf( cmd, "./geth attach http://localhost:8545 --exec \"var value=web3.eth.getBalance('%s'); web3.fromWei(value, 'gwei');\" " , mineraccount);
//    printf("%s\n", cmd);

   in=popen(cmd, "r");
   fgets(temp, 255, in);
   pclose(in);

	balance = strtoll(temp, NULL, 10);  
//	balance = atoi(temp) - 1;
   printf("%s : %s , Balance : %ld gwei \n", mineraccount,temp, balance);
   return balance;
}

void printminerslist(char minerlist[500][256],int minercnt)
{
	char miner[256];
	int64_t rval;
	for (int i = 0 ; i < minercnt; i++)
	{
		char *pminerstr = minerlist[i];
		strncpy(miner, pminerstr+11, 42);
		miner[42] = '\0';
		rval = return_node_balance(miner);
		printf("miner %03d [%s], Val:%ld\n", i, miner, rval);
	}
}


int main(int argc, char **argv) {
    unsigned int i, j;
    redisContext *c;
    redisReply *reply;
    const char *hostname = "127.0.0.1";
    int port = 6379;
	int db = 0;
	const char *password = "";
    const char cmdstr[512];
    const char minerslist[500][256] = { 0, };
	int index, minercnt;
	int oc, wi = 0;

	opterr = 0;
	while ((oc = getopt (argc, argv, "h:p:d:a:s")) != -1) 
	{
    switch (oc)
      {
      case 'h':
        hostname = optarg;
        break;
      case 'p':
        port = atoi(optarg);
        break;
      case 'd':
        db = atoi(optarg);
        break;
      case 'a':
	password = optarg;
	break;
      default:
        break;
      }
	}
  	printf ("hostname = %s, port = %d, db = %d, password %s\n",
          hostname, port, db, password);

//     print_whitelist(g_whitelist);

    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    c = redisConnectWithTimeout(hostname, port, timeout);
    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }

    /* PING server */
    reply = redisCommand(c,"PING");
    printf("PING: %s\n", reply->str);
    freeReplyObject(reply);

    /* AUTH server */
    if (password) {
    reply = redisCommand(c,"AUTH %s", password);
    printf("AUTH: %s\n", reply->str);
    freeReplyObject(reply);
    }

    /* Set DB */
    reply = redisCommand(c,"SELECT %d", db);
    printf("SELECT: %s\n", reply->str);
    freeReplyObject(reply);

    /* Search Key miners */
    reply = redisCommand(c,"KEYS  *teo:miners* ");
    // printf("Type : %d, len : %d, SCAN Elements: %d \n", reply->type, reply->len, reply->elements);
    if (reply->type == REDIS_REPLY_ERROR ) {
        printf("R.Return Error; %s\n", reply->str);
        //freeReplyObject(reply);
    }

    minercnt = reply->elements;
    for (i = 0; i < reply->elements ; i++)
	{
        // printf("Elm # %02d : [%s]\n", i, reply->element[i]->str);
        strcpy( minerslist[i], reply->element[i]->str );
	}
    freeReplyObject(reply);


    for (i = 0; i < minercnt ; i++)
    {
        int64_t int64array[5]; // max 5 : buffer for reserved 5
	int64_t rval;
	double paidteo;
	int64_t paydiff;
	char	miner[256];
        /* Scan miner Key Lists */
        /* compare miner is in whitelist or not */
        MinerBalance minerval;

        //if (check_whitelisted(minerslist[i], g_whitelist )){
        //    continue;
        //}

        sprintf(cmdstr,"HMGET  %s balance immature bakbalance bakimmature paid", minerslist[i]);
        // printf("%s\n", cmdstr);
        reply = redisCommand(c, cmdstr );
        if (reply->type == REDIS_REPLY_ERROR )
        {
            printf("R.Return Error: %s\n", reply->str);
        }
        // printf("Type : %d, len : %d, SCAN Elements: %d \n", reply->type, reply->len, reply->elements);

        for (j = 0; j < reply->elements ; j++)
        {
            int64array[j] = (reply->element[j]->str) ? (atoll(reply->element[j]->str)) : 0L;
            // printf("ARR : Type:%d , Elm # %02d : Len:%d [%s] %ld\n", reply->type, j, reply->len, reply->element[j]->str, int64array[j]);
        }
        freeReplyObject(reply);

        copyMinerKey( &minerval, &int64array);

	// if (minerval.bakbalance || minerval.bakimmature)	
	paidteo = (double)(minerval.paid/1000000000.0);
       	printf("%s:%lf:(%ld,%ld, %ld, %ld, %ld) \n", minerslist[i],  paidteo, minerval.balance, minerval.immature, minerval.bakbalance, minerval.bakimmature, minerval.paid );

	{
		char *pminerstr = minerslist[i];
		strncpy(miner, pminerstr+11, 42);
		miner[42] = '\0';
	}
	rval = return_node_balance(miner);
	paydiff = minerval.paid - rval;
	printf(" PoolPaid : %ld , WalletVal : %ld ,  Diff[p-r] [%ld] %lf TEO\n" , minerval.paid, rval, paydiff, ((paydiff)/1000000000.0));


    	/* Search  miner payments */
	/*
	if (abs(paydiff) > 500000000) {
		int listcnt;
    		const char cmdstr1[512];
		// dump all transaction lists with time stamp
		sprintf(cmdstr1, "ZCARD teo:payments:%s", miner);
    		reply = redisCommand(c,cmdstr1);
    		if (reply->type == REDIS_REPLY_ERROR ) {
			printf("R.Return Error; %s\n", reply->str);
		        //freeReplyObject(reply);
   	 	}
		
		listcnt = reply->integer;
		printf("%s : Transaction Count : %d", cmdstr1,listcnt);
		freeReplyObject(reply);

		sprintf(cmdstr1, "ZRANGE teo:payments:%s 0 %d WITHSCORES", miner, listcnt);
    		reply = redisCommand(c,cmdstr1);
		for (int j = 0 ; j < reply->elements ; j ++) {
			printf("\t[%03d] %s\n", j, reply->element[j]->str);
		}
		freeReplyObject(reply);
	}
	*/

        /* move pending balance to bakbalance */
 #if 0
        sprintf(cmdstr,"HMSET  %s balance %ld immature %ld bakbalance %ld bakimmature %ld ", \
                        minerslist[i],          \
                        minerval.balance,minerval.immature,     \
                        minerval.bakbalance, minerval.bakimmature );

        printf("%s\n", cmdstr);
        reply = redisCommand(c, cmdstr );
        if (reply->type == REDIS_REPLY_ERROR )
        {
            printf("R.Return Error: %s\n", reply->str);
        }
        freeReplyObject(reply);
 #endif        
    }
        
//    printminerslist(minerslist, minercnt);

#if 0 

    /* Get Miner Lists */
    reply = redisCommand(c,"HSCAN %s * ", "eth:miners" );
    printf("SCAN MINERS: %s\n", reply->str);
    freeReplyObject(reply);
#endif

#if 0
#define REDIS_REPLY_STRING 1
#define REDIS_REPLY_ARRAY 2
#define REDIS_REPLY_INTEGER 3
#define REDIS_REPLY_NIL 4
#define REDIS_REPLY_STATUS 5
#define REDIS_REPLY_ERROR 6

/* This is the reply object returned by redisCommand() */
typedef struct redisReply {
    int type; /* REDIS_REPLY_* */
    long long integer; /* The integer when type is REDIS_REPLY_INTEGER */
    size_t len; /* Length of string */
    char *str; /* Used for both REDIS_REPLY_ERROR and REDIS_REPLY_STRING */
    size_t elements; /* number of elements, for REDIS_REPLY_ARRAY */
    struct redisReply **element; /* elements vector for REDIS_REPLY_ARRAY */
} redisReply;


    /* Set a key */
    reply = redisCommand(c,"SET %s %s", "foo", "hello world");
    printf("SET: %s\n", reply->str);
    freeReplyObject(reply);

    /* Set a key using binary safe API */
    reply = redisCommand(c,"SET %b %b", "bar", (size_t) 3, "hello", (size_t) 5);
    printf("SET (binary API): %s\n", reply->str);
    freeReplyObject(reply);

    /* Try a GET and two INCR */
    reply = redisCommand(c,"GET foo");
    printf("GET foo: %s\n", reply->str);
    freeReplyObject(reply);

    reply = redisCommand(c,"INCR counter");
    printf("INCR counter: %lld\n", reply->integer);
    freeReplyObject(reply);
    /* again ... */
    reply = redisCommand(c,"INCR counter");
    printf("INCR counter: %lld\n", reply->integer);
    freeReplyObject(reply);

    /* Create a list of numbers, from 0 to 9 */
    reply = redisCommand(c,"DEL mylist");
    freeReplyObject(reply);
    for (j = 0; j < 10; j++) {
        char buf[64];

        snprintf(buf,64,"%u",j);
        reply = redisCommand(c,"LPUSH mylist element-%s", buf);
        freeReplyObject(reply);
    }
    /* Let's check what we have inside the list */
    reply = redisCommand(c,"LRANGE mylist 0 -1");
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (j = 0; j < reply->elements; j++) {
            printf("%u) %s\n", j, reply->element[j]->str);
        }
    }
    freeReplyObject(reply);
#endif

    /* Disconnects and frees the context */
    redisFree(c);

    return 0;
}
