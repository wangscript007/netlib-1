/*
 * Connector.h
 *
 *  Created on: Jan 16,2016
 *      Author: zhangyalei
 */

#ifndef LIB_CONNECTOR_H_
#define LIB_CONNECTOR_H_

#include "Connect.h"
#include "Pack.h"
#include "Receive.h"
#include "Send.h"
#include "Svc.h"
#include "Svc_Static_List.h"
#include "Thread_Mutex.h"
#include "Object_Pool.h"
#include "Block_Pool_Group.h"

class Connector_Connect: public Connect {
public:
	virtual int connect_svc(int connfd);
};

////////////////////////////////////////////////////////////////////////////////

class Connector_Svc: public Svc {
public:
	virtual Block_Buffer *pop_block(int cid);

	virtual int push_block(int cid, Block_Buffer *block);

	virtual int register_recv_handler(void);

	virtual int unregister_recv_handler(void);

	virtual int register_send_handler(void);

	virtual int unregister_send_handler(void);

	virtual int recv_handler(int cid);

	virtual int close_handler(int cid);
};

////////////////////////////////////////////////////////////////////////////////

class Connector_Receive: public Receive {
public:
	virtual int drop_handler(int cid);

	virtual Svc *find_svc(int cid);
};

////////////////////////////////////////////////////////////////////////////////

class Connector_Send: public Send {
public:
	/// 获取、释放一个buf
	virtual Block_Buffer *pop_block(int cid);

	virtual int push_block(int cid, Block_Buffer *buf);

	virtual int drop_handler(int cid);

	virtual Svc *find_svc(int cid);
};

////////////////////////////////////////////////////////////////////////////////

class Connector_Pack: public Pack {
public:
	virtual Svc *find_svc(int cid);

	virtual Block_Buffer *pop_block(int cid);

	virtual int push_block(int cid, Block_Buffer *block);

	virtual int packed_data_handler(Block_Vector &block_vec);

	virtual int drop_handler(int cid);
};

////////////////////////////////////////////////////////////////////////////////

class Connector: public Thread {
public:
	typedef Object_Pool<Connector_Svc, Spin_Lock> Client_Svc_Pool;
	typedef Svc_Static_List<Connector_Svc *, Spin_Lock> Svc_List;
	typedef Block_List<Thread_Mutex> Data_List;

	static const int svc_max_list_size = 20000; /// 20000个包
	static const int svc_max_pack_size = 60 * 1024; /// 接收单个包最大60K

	Connector(void);
	virtual ~Connector(void);

	void run_handler(void);
	virtual void process_list(void);

	void set(std::string ip, int port, Time_Value &send_interval);
	int init(void);
	int start(void);
	int connect_server(void);

	inline Svc_List &svc_list(void) { return svc_static_list_; }
	inline Client_Svc_Pool &svc_pool(void) { return svc_pool_; }
	inline Data_List &block_list(void) { return block_list_;}
	inline Connector_Receive &receive(void) { return receive_; }
	inline Connector_Send &send(void) { return send_; }
	inline Connector_Pack &pack(void) { return pack_; }
	inline int get_cid(void) { return cid_; }

	Block_Buffer *pop_block(int cid);
	int push_block(int cid, Block_Buffer *block);
	int send_block(int cid, Block_Buffer &buf);

	Svc *find_svc(int cid);
	int recycle_svc(int cid);

	int get_server_info(Server_Info &info);
	void free_cache(void);

protected:
	Data_List block_list_;

private:
	Svc_List svc_static_list_;
	Client_Svc_Pool svc_pool_;
	Block_Pool_Group block_pool_group_;

	Connector_Connect connect_;
	Connector_Receive receive_;
	Connector_Send send_;
	Connector_Pack pack_;

	int32_t cid_;
	std::string ip_;
	int port_;
};

#endif /* LIB_CONNECTOR_H_ */
