#ifndef __MSG_EX_H__
#define __MSG_EX_H__

#include "Lunar.h"
#include <string.h>
#include <assert.h>


#define MAX_BUF_LEN 50

class Msg {
public:
	Msg(int id){
		m_msgId = id;
		m_buf = new char[MAX_BUF_LEN];
		memset(m_buf, '\0', sizeof(m_buf));
	}
	~Msg(){
		delete[] m_buf;
		m_buf = NULL;
	}

	void ToString() {
		printf("msgId: %d===buf: %s\n", m_msgId, m_buf);
	}
public: //for simplifying
	int m_msgId;
	char* m_buf;
};


class MsgEx
{
    public:
		MsgEx(){
			m_pMsg = new Msg(0);
        }
		~MsgEx(){
			delete m_pMsg;
			m_pMsg = NULL;
		}
        inline Msg* GetMsg(){
			return m_pMsg;
        }

        inline void SetMsg(Msg* pMsg){
			m_pMsg = pMsg;
        }

    private:
        Msg *m_pMsg;
};


struct lua_State;
class LuaMsgEx
{
    public:
        LuaMsgEx(lua_State *pL){
            m_pMsgEx = (MsgEx*)lua_touserdata(pL, -1);
        }

        int ReadMsg(lua_State* pL);
		int WriteMsg(lua_State* pL);
		int Print(lua_State* pL);
		void NotRegisted();

        const static char className [] ;        
        static Lunar<LuaMsgEx>::RegType methods[];

    private:
        MsgEx* m_pMsgEx;
};


#endif //__MSG_EX_H__
