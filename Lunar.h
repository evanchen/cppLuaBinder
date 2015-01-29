#ifndef __LUNAR_H__
#define __LUNAR_H__

/*
 * Copied from:
 * http://lua-users.org/wiki/CppBindingWithLunar
 */


extern "C"{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
};

template <typename T> class Lunar {
  typedef struct { T *pT; } userdataType;
public:
  typedef int (T::*mfp)(lua_State *L);
  typedef struct { const char *name; mfp mfunc; } RegType;

  static void Register(lua_State *L) {
    lua_newtable(L); //1.���������ע��table����,����������ǵİ�����
    int methods = lua_gettop(L); //2.��¼����½�table��index

    luaL_newmetatable(L, T::className); //3.����һ��userdataԪ��������Ϊ˭��Ԫ�������ٿ�
    int metatable = lua_gettop(L);//4.��¼Ԫ��index

    // store method table in globals so that
    // scripts can add functions written in Lua.
    lua_pushvalue(L, methods); //5.������table����ѹ��ջ��
    set(L, LUA_GLOBALSINDEX, T::className);//6.����������ֺ͸���ע�ᵽlua�������G[T:className] = methods,��ʱջ����ԭ��4

    // hide metatable from Lua getmetatable()
    lua_pushvalue(L, methods);
    set(L, metatable, "__metatable"); //7.ͬ��metatable["__metatable"] = methods

    lua_pushvalue(L, methods);
    set(L, metatable, "__index");//8.metatable["__index"] = methods

    lua_pushcfunction(L, tostring_T);
    set(L, metatable, "__tostring"); //9.metatable["__tostring"] = tostring_T

    lua_pushcfunction(L, gc_T);
    set(L, metatable, "__gc"); //10.metatable["__gc"] = gc_T

	//mt
    lua_newtable(L);                // 11.mt for method table
    lua_pushcfunction(L, new_T);	// 12.��new_T�ŵ�ջ����������������ð���Ĺ��캯��
    lua_pushvalue(L, -1);           // 13.����һ��new_T������ջ��
    set(L, methods, "new");         // 14.methods["new"] = new_T,��ʱջ������һ��new_T
    set(L, -3, "__call");           // 15.index = -3������ȥset��������ΪҪpushstring,���Զ�Ӧ����11������mt,����mt["__call"] = new_T
    lua_setmetatable(L, methods);	// 16.��ջ����mt����Ϊmethods��Ԫ��methods["__metatable"] = mt,��ʱmt��ջ��ջ������4������metatable��

    // fill method table with methods from class T��17.ע���Ա���������ǵ��Ǹ�������Ǹ���̬�����Ա������
    for (RegType *l = T::methods; l->name; l++) {
      lua_pushstring(L, l->name); 
      lua_pushlightuserdata(L, (void*)l); //18.�Ѷ�Ӧ�ĺ���ѹջ
      lua_pushcclosure(L, thunk, 1);//19.�����thunk����ѹ��ջ�������Ұ�18���������Ϊ����upvalue��18����������ջ���Ժ����ǻ�֪��ΪʲôҪ������
      lua_settable(L, methods);//20.���ˣ��Ѷ�Ӧ�ĺ�������thunk����set��methods��ȥ�����֮��methods���lua����Ϳ��Է��ʰ���ĳ�Ա������
    }

    lua_pop(L, 2);  // drop metatable and method table, 21.��metatable��methods����table����ջ
  }

  // call named lua method from userdata method table
  static int call(lua_State *L, const char *method,
                  int nargs=0, int nresults=LUA_MULTRET, int errfunc=0)
  {
    int base = lua_gettop(L) - nargs;  // userdata index
    if (!luaL_checkudata(L, base, T::className)) {
      lua_settop(L, base-1);           // drop userdata and args
      lua_pushfstring(L, "not a valid %s userdata", T::className);
      return -1;
    }

    lua_pushstring(L, method);         // method name
    lua_gettable(L, base);             // get method from userdata
    if (lua_isnil(L, -1)) {            // no method?
      lua_settop(L, base-1);           // drop userdata and args
      lua_pushfstring(L, "%s missing method '%s'", T::className, method);
      return -1;
    }
    lua_insert(L, base);               // put method under userdata, args

    int status = lua_pcall(L, 1+nargs, nresults, errfunc);  // call method
    if (status) {
      const char *msg = lua_tostring(L, -1);
      if (msg == NULL) msg = "(error with no message)";
      lua_pushfstring(L, "%s:%s status = %d\n%s",
                      T::className, method, status, msg);
      lua_remove(L, base);             // remove old message
      return -1;
    }
    return lua_gettop(L) - base + 1;   // number of results
  }

  // push onto the Lua stack a userdata containing a pointer to T object
  static int push(lua_State *L, T *obj, bool gc=false) {
    if (!obj) { lua_pushnil(L); return 0; }
    luaL_getmetatable(L, T::className);  // lookup metatable in Lua registry, ����Register()������ע���metatable�������ŵ�ջ��
    if (lua_isnil(L, -1)) luaL_error(L, "%s missing metatable", T::className);
    int mt = lua_gettop(L); //��metatable��index�ó���
    subtable(L, mt, "userdata", "v"); //����metatable��û��metatable["userdata"]��û�еĻ�����һ��table��������Ԫ��ֵΪ�����ã��������table��Ԫ�������Լ�
	//ע����ʱջ����metatable["userdata"]
    userdataType *ud =
      static_cast<userdataType*>(pushuserdata(L, obj, sizeof(userdataType)));//��ȡmetatable["userdata"][obj]��û�оʹ���һ��userdata
	//��ʱջ����metatable["userdata"][obj]
    if (ud) {
      ud->pT = obj;  // store pointer to object in userdata,�����ǵ�c++����ָ��󶨵�userdata��
      lua_pushvalue(L, mt); //metatable�ٴ���ջ
      lua_setmetatable(L, -2);//��metatable����Ϊmetatable["userdata"][obj]��Ԫ��,metatable����
      if (gc == false) { //����½���userdata����gc
        lua_checkstack(L, 3);
        subtable(L, mt, "do not trash", "k"); //metatable["do not trash"]��Ԫ��keyΪ�����ã��������table��Ԫ�������Լ�����ʱջ����metatable["do not trash"]
		lua_pushvalue(L, -2); //��XX = metatable["userdata"][obj]������ջ��
        lua_pushboolean(L, 1);//��ѹ��һ��trueֵ
        lua_settable(L, -3); //metatable["do not trash"][XX] = true������Ϊ��gc��
        lua_pop(L, 1); //����metatable["do not trash"]
      }
    }
    lua_replace(L, mt); //��metatable["userdata"][obj]�������������滻��mtջλ��Ԫ��
    lua_settop(L, mt); //��ջ��ָ���ƶ���mt,�൱�ڵ�����metatable["userdata"]�����ڵ�ջ��metatable["userdata"][obj]�����������ظ�lua��ĵ�����
    return mt;  // index of userdata containing pointer to T object
  }

  // get userdata from Lua stack and return pointer to T object
  static T *check(lua_State *L, int narg) {
    userdataType *ud =
      static_cast<userdataType*>(luaL_checkudata(L, narg, T::className)); //���L��nargλ���Ƿ���userdata��Ȼ����������metatableָ����Register()ע���metatable�Ƚ�
    if(!ud) {
        luaL_typerror(L, narg, T::className);
        return NULL;
    }
    return ud->pT;  // pointer to T object
  }

private:
  Lunar();  // hide default constructor

  // member function dispatcher
  static int thunk(lua_State *L) {
    // stack has userdata, followed by method args
    T *obj = check(L, 1);  // get 'self', or if you prefer, 'this',��ʵ�����ó�lua���LuaMsgEx��userdata���ٷ���userdataType->pT,Ҳ�������ǵ���new_T���ɵ�T*
    lua_remove(L, 1);  // remove self so member function args start at index 1, ����ջ����self��ʣ��ľ���������ò���
    // get member function from upvalue
    RegType *l = static_cast<RegType*>(lua_touserdata(L, lua_upvalueindex(1)));//�ó�thunk������upvalue����������ע���Ա��������Ϣ
    return (obj->*(l->mfunc))(L);  // call member function, Ȼ����ǵ���c++����ĳ�Ա�����ˣ�ע��L�Ѿ��Ƴ���lua���selfŶ
  }

  // create a new T object and
  // push onto the Lua stack a userdata containing a pointer to T object
  static int new_T(lua_State *L) {
    lua_remove(L, 1);   // use classname:new(), instead of classname.new(),��һ��������self���Ƴ���
    T *obj = new T(L);  // call constructor for T objects
    push(L, obj, true); // gc_T will delete this object, ����gc����ʱ�Ƿ��ܱ�delete
    return 1;           // userdata containing pointer to T object
  }

  // garbage collection metamethod
  static int gc_T(lua_State *L) {
    if (luaL_getmetafield(L, 1, "do not trash")) {
      lua_pushvalue(L, 1);  // dup userdata
      lua_gettable(L, -2);
      if (!lua_isnil(L, -1)) return 0;  // do not delete object
    }
    userdataType *ud = static_cast<userdataType*>(lua_touserdata(L, 1));
    T *obj = ud->pT;
    if (obj) delete obj;  // call destructor for T objects
    return 0;
  }

  static int tostring_T (lua_State *L) {
    char buff[32];
    userdataType *ud = static_cast<userdataType*>(lua_touserdata(L, 1));
    T *obj = ud->pT;
    sprintf(buff, "%p", (void*)obj);
    lua_pushfstring(L, "%s (%s)", T::className, buff);

    return 1;
  }

  static void set(lua_State *L, int table_index, const char *key) {
    lua_pushstring(L, key); //keyѹ��ջ��
    lua_insert(L, -2);  // swap value and key����ջ����key����֮�µ�Ԫ��λ�ý���
    lua_settable(L, table_index);//��[key] = value����ָ����table_index��������key��value
  }

  static void weaktable(lua_State *L, const char *mode) {
    lua_newtable(L);
    lua_pushvalue(L, -1);  // table is its own metatable
    lua_setmetatable(L, -2);
    lua_pushliteral(L, "__mode");
    lua_pushstring(L, mode);
    lua_settable(L, -3);   // metatable.__mode = mode
  }

  static void subtable(lua_State *L, int tindex, const char *name, const char *mode) {
    lua_pushstring(L, name);
    lua_gettable(L, tindex);
    if (lua_isnil(L, -1)) {
      lua_pop(L, 1);
      lua_checkstack(L, 3);
      weaktable(L, mode);
      lua_pushstring(L, name);
      lua_pushvalue(L, -2);//�½���weaktable�ŵ�ջ��
      lua_settable(L, tindex);//stack[tindex][name] = weaktable, ����������ջ��
    }
  }

  static void *pushuserdata(lua_State *L, void *key, size_t sz) {
    void *ud = 0;
    lua_pushlightuserdata(L, key);
    lua_gettable(L, -2);     // lookup[key],����metatable["userdata"]��û��ע�ᵽ���key
    if (lua_isnil(L, -1)) {
      lua_pop(L, 1);         // drop nil
      lua_checkstack(L, 3);
      ud = lua_newuserdata(L, sz);  // create new userdata, ����һ��ָ���С��userdata
      lua_pushlightuserdata(L, key); //��keyѹջ
      lua_pushvalue(L, -2);  // dup userdata,��userdata�ŵ�ջ��
      lua_settable(L, -4);   // lookup[key] = userdata, ��metatable["userdata"][key] = �����½���userdata, ����������ջ��
    }
    return ud;
  }
};

#define LUNAR_DECLARE_METHOD(Class, Name) {#Name, &Class::Name}

#endif //__LUNAR_H__

