/*
 * Transaction.hpp
 *
 *  Created on: Jan 23, 2017
 *      Author: anhmt90
 */
/*
 * Abort CODE:
 * 				1 = commit succeeds
 * 				-1 = Vissibility validation failed
 * 				-2 = Phantom validation failed
 * 				-19 = cascaded abort by commit dependency
 *
 */


#ifndef SRC_TRANSACTION_HPP_
#define SRC_TRANSACTION_HPP_

#include <climits>	//for CHAR_BIT
#include <set>
#include <cstdint>
#include <atomic>
#include <thread>

#include "Schema.hpp"

using namespace std;
typedef unordered_multimap<Integer, Warehouse::Tuple> Warehouse_PK;
typedef unordered_multimap<tup_4Int, OrderLine::Tuple> OrderLine_PK;
/****************************************************************************/
struct Transaction;

// value to present a null value of a field
const uint64_t NOT_SET = 1ull<<63;

// counter to generate the Transactions ID
extern atomic<uint64_t> GMI_tid;

// container to manage all running Transactions
extern unordered_map<uint64_t,Transaction*> TransactionManager;

// vector to store terminated Transactions
extern vector<Transaction*> GarbageTransactions;

/****************************************************************************/
uint64_t getTid();
/****************************************************************************/

class TransactionNotFoundException : public exception {
public:
	TransactionNotFoundException(){}
};


struct Predicate {
	Integer pk_int;
	tup_2Int pk_2int;
	tup_3Int pk_3int;
	tup_4Int pk_4int;

	Predicate(Integer pk_int) : pk_int(pk_int){

	}

	Predicate(tup_2Int pk_2int) : pk_2int(pk_2int){

	}

	Predicate(tup_3Int pk_3int) : pk_3int(pk_3int){

	}

	Predicate(tup_4Int pk_4int) : pk_4int(pk_4int){

	}

	~Predicate(){ };
};




struct Transaction{
	// Transaction ID
	const uint64_t Tid;
	// begin time of the transaction, also used as logical read time
	const uint64_t begin;
	/*
	 *  end time of the transaction, will be set as INF in the constructor,
	 *  will be set by getTimestamp() again when the transaction precommits.
	 */
	uint64_t end;

	/*
	 * counts how many unresolved commit dependencies this transaction still has.
	 * A transaction cannot commit until this counter = 0
	 */
	int CommitDepCounter = 0;

	/*
	 * other transactions can set this variable to tell this transaction
	 * to abort.
	 */
	bool AbortNow = false;

	//terminate code
	int CODE;

	/* each transaction can be in 1 of 4 states */
	enum class State : unsigned {Active, Preparing, Committed, Aborted};
	State state;

	/*
	 * stores Tid of the transactions that depend on this transaction
	 */
	vector<uint64_t> CommitDepSet;

	/*
	 *	stores the Versions read, not ones updated
	 */
	vector<Version*> ReadSet;

	vector<pair<Warehouse_PK*,Integer>> ScanSet_Warehouse;
	vector<pair<OrderLine_PK*,tup_4Int>> ScanSet_OrderLine;
	/*
	 * first: old version
	 * second: new version
	 * if second  = nullptr, then it is a delete
	 */
	vector<pair<Version*, Version*>> WriteSet;

	Transaction(int i):Tid(getTid()),begin(getTimestamp()), end(1ull<<63), state(State::Active){
		TransactionManager.insert(make_pair(Tid, this));
		execute(i);
	}

	~Transaction(){ };

//	void setState(State state){
//		this->state = state;
//	}
	void decreaseCommitDepCounter();

	int checkVisibility(Version&);
	int checkUpdatibility(Version&);

	void execute(int);

	Version* read(string, Predicate, bool);
	Version* update(string, Predicate, bool);
	Version* insert(string tableName, Version* VI, Predicate pred);

	void precommit();
	void abort(int);
	void commit();

	int validate();


};


//struct TransactionManager{
//	unordered_map<uint64_t, Transaction> transactions;
//};


#endif /* SRC_TRANSACTION_HPP_ */
