
g_MsgEx = g_MsgEx or LuaMsgEx(_Msg)

function NewMsg() 
	local o = {
		id = 0,
		buf = ""
	}
	
	return o
end 

function Init()
	-- write 
	local msg1 = NewMsg()
	msg1.id = 111
	msg1.buf = "hello world"
	
	g_MsgEx:WriteMsg(msg1)
	print("write:")
	g_MsgEx:Print()
	
	--read 
	local msg2 = NewMsg()
	g_MsgEx:ReadMsg(msg2)
	
	print("read")
	for k,v in pairs(msg2) do 
		print(k,v)
	end 
	
	g_MsgEx = nil
	collectgarbage("collect")
end 
