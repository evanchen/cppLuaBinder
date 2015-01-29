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
    lua_newtable(L); //1.在虚拟机上注册table对象,这个就是我们的绑定类了
    int methods = lua_gettop(L); //2.记录这个新建table的index

    luaL_newmetatable(L, T::className); //3.创建一个userdata元表，到底作为谁的元表，等下再看
    int metatable = lua_gettop(L);//4.记录元表index

    // store method table in globals so that
    // scripts can add functions written in Lua.
    lua_pushvalue(L, methods); //5.将绑定类table副本压入栈顶
    set(L, LUA_GLOBALSINDEX, T::className);//6.将绑定类的名字和副本注册到lua虚拟机的G[T:className] = methods,此时栈顶还原到4

    // hide metatable from Lua getmetatable()
    lua_pushvalue(L, methods);
    set(L, metatable, "__metatable"); //7.同理，metatable["__metatable"] = methods

    lua_pushvalue(L, methods);
    set(L, metatable, "__index");//8.metatable["__index"] = methods

    lua_pushcfunction(L, tostring_T);
    set(L, metatable, "__tostring"); //9.metatable["__tostring"] = tostring_T

    lua_pushcfunction(L, gc_T);
    set(L, metatable, "__gc"); //10.metatable["__gc"] = gc_T

	//mt
    lua_newtable(L);                // 11.mt for method table
    lua_pushcfunction(L, new_T);	// 12.把new_T放到栈顶，后面会用来调用绑定类的构造函数
    lua_pushvalue(L, -1);           // 13.拷贝一个new_T副本到栈顶
    set(L, methods, "new");         // 14.methods["new"] = new_T,这时栈顶还有一个new_T
    set(L, -3, "__call");           // 15.index = -3，传进去set函数后因为要pushstring,所以对应的是11创建的mt,于是mt["__call"] = new_T
    lua_setmetatable(L, methods);	// 16.把栈顶的mt设置为methods的元表methods["__metatable"] = mt,此时mt出栈，栈顶就是4创建的metatable了

    // fill method table with methods from class T，17.注册成员函数，还记得那个绑定类的那个静态数组成员变量吧
    for (RegType *l = T::methods; l->name; l++) {
      lua_pushstring(L, l->name); 
      lua_pushlightuserdata(L, (void*)l); //18.把对应的函数压栈
      lua_pushcclosure(L, thunk, 1);//19.这里把thunk函数压入栈顶，并且把18这个函数作为它的upvalue，18这个函数会出栈。稍后我们会知道为什么要这样做
      lua_settable(L, methods);//20.好了，把对应的函数名和thunk函数set到methods里去，完成之后，methods这个lua对象就可以访问绑定类的成员函数了
    }

    lua_pop(L, 2);  // drop metatable and method table, 21.把metatable和methods两个table弹出栈
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
    luaL_getmetatable(L, T::className);  // lookup metatable in Lua registry, 查找Register()函数里注册的metatable，把它放到栈顶
    if (lua_isnil(L, -1)) luaL_error(L, "%s missing metatable", T::className);
    int mt = lua_gettop(L); //把metatable的index拿出来
    subtable(L, mt, "userdata", "v"); //查找metatable有没有metatable["userdata"]，没有的话创建一个table并设置它元素值为弱引用，并且这个table的元表是它自己
	//注意这时栈顶是metatable["userdata"]
    userdataType *ud =
      static_cast<userdataType*>(pushuserdata(L, obj, sizeof(userdataType)));//获取metatable["userdata"][obj]，没有就创建一个userdata
	//此时栈顶是metatable["userdata"][obj]
    if (ud) {
      ud->pT = obj;  // store pointer to object in userdata,将我们的c++对象指针绑定到userdata里
      lua_pushvalue(L, mt); //metatable再次入栈
      lua_setmetatable(L, -2);//将metatable设置为metatable["userdata"][obj]的元表,metatable弹出
      if (gc == false) { //如果新建的userdata不被gc
        lua_checkstack(L, 3);
        subtable(L, mt, "do not trash", "k"); //metatable["do not trash"]的元素key为弱引用，并且这个table的元表是它自己。此时栈顶是metatable["do not trash"]
		lua_pushvalue(L, -2); //把XX = metatable["userdata"][obj]拷贝到栈顶
        lua_pushboolean(L, 1);//再压入一个true值
        lua_settable(L, -3); //metatable["do not trash"][XX] = true，这是为了gc啊
        lua_pop(L, 1); //弹出metatable["do not trash"]
      }
    }
    lua_replace(L, mt); //把metatable["userdata"][obj]弹出，并把它替换掉mt栈位置元素
    lua_settop(L, mt); //把栈顶指针移动到mt,相当于弹出了metatable["userdata"]，现在的栈顶metatable["userdata"][obj]，它将被返回给lua层的调用者
    return mt;  // index of userdata containing pointer to T object
  }

  // get userdata from Lua stack and return pointer to T object
  static T *check(lua_State *L, int narg) {
    userdataType *ud =
      static_cast<userdataType*>(luaL_checkudata(L, narg, T::className)); //检查L的narg位置是否是userdata，然后再拿它的metatable指针与Register()注册的metatable比较
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
    T *obj = check(L, 1);  // get 'self', or if you prefer, 'this',其实就是拿出lua层的LuaMsgEx的userdata，再返回userdataType->pT,也就是我们调用new_T生成的T*
    lua_remove(L, 1);  // remove self so member function args start at index 1, 调用栈弹出self后，剩余的就是其余调用参数
    // get member function from upvalue
    RegType *l = static_cast<RegType*>(lua_touserdata(L, lua_upvalueindex(1)));//拿出thunk函数的upvalue，就是我们注册成员函数的信息
    return (obj->*(l->mfunc))(L);  // call member function, 然后就是调用c++对象的成员函数了，注意L已经移除了lua层的self哦
  }

  // create a new T object and
  // push onto the Lua stack a userdata containing a pointer to T object
  static int new_T(lua_State *L) {
    lua_remove(L, 1);   // use classname:new(), instead of classname.new(),第一个参数是self，移除掉
    T *obj = new T(L);  // call constructor for T objects
    push(L, obj, true); // gc_T will delete this object, 设置gc调用时是否能被delete
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
    lua_pushstring(L, key); //key压入栈顶
    lua_insert(L, -2);  // swap value and key，把栈顶的key和它之下的元素位置交换
    lua_settable(L, table_index);//将[key] = value塞到指定的table_index，并弹出key和value
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
      lua_pushvalue(L, -2);//新建的weaktable放到栈顶
      lua_settable(L, tindex);//stack[tindex][name] = weaktable, 并且它留在栈顶
    }
  }

  static void *pushuserdata(lua_State *L, void *key, size_t sz) {
    void *ud = 0;
    lua_pushlightuserdata(L, key);
    lua_gettable(L, -2);     // lookup[key],看看metatable["userdata"]有没有注册到这个key
    if (lua_isnil(L, -1)) {
      lua_pop(L, 1);         // drop nil
      lua_checkstack(L, 3);
      ud = lua_newuserdata(L, sz);  // create new userdata, 创建一个指针大小的userdata
      lua_pushlightuserdata(L, key); //把key压栈
      lua_pushvalue(L, -2);  // dup userdata,把userdata放到栈顶
      lua_settable(L, -4);   // lookup[key] = userdata, 把metatable["userdata"][key] = 这里新建的userdata, 并且它留在栈顶
    }
    return ud;
  }
};

#define LUNAR_DECLARE_METHOD(Class, Name) {#Name, &Class::Name}

#endif //__LUNAR_H__

