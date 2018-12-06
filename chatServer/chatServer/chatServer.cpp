// chatServer.cpp : 定义控制台应用程序的入口点。
//

#pragma once
#include "stdafx.h"
#include "string"
#include "list"
#include "windows.h"
#include "process.h"
#include "iostream"
#include "fstream"


using namespace std;
#pragma comment(lib,"ws2_32.lib")

enum Message_Type
{
	Message_Min,
	Message_Login,
	Message_LoginOut,
	Message_Chat,
	Message_ChatToMe,
	Message_ChatToOther,
	Message_File,
	Message_Max
};

struct stUserInfo
{
	SOCKET sock;
	wstring strName;
};

list<stUserInfo> g_listUser;	//保存所有在线的用户信息

//判断列表里是否存在了该名字
bool IsExist(wstring name)
{
	for (list<stUserInfo>::iterator it = g_listUser.begin(); it != g_listUser.end(); it++)
	{
		if (it->strName == name)
		{
			return true;
		}
	}
	return false;
}

wstring GetNewName(stUserInfo userInfo)
{
	wchar_t newName[100];
	int index = 1;
	while(1)
	{
		//由于需要'_'做分割，这里电脑名有'_'会出问题
		wsprintf(newName,L"%s%d",userInfo.strName.c_str(),index);
		if (!IsExist(newName))
		{
			break;
		}
		index++;
	}
	wstring str = newName;
	return str;
}

void SendToClient(Message_Type msgType,SOCKET sock,wstring str)
{
	if (msgType <= Message_Min || msgType >= Message_Max)
	{
		return;
	}
	wchar_t sendBuf[1024];
	switch(msgType)
	{
	case Message_Login:
		{
			wsprintf(sendBuf,L"%d_%s",msgType,str.c_str());
			break;
		}
	case Message_LoginOut:
		{
			wsprintf(sendBuf,L"%d_%s",msgType,str.c_str());
			break;
		}
	case Message_ChatToMe:
	case Message_ChatToOther:
		{
			wsprintf(sendBuf,L"%d_%s",msgType,str.c_str());
			break;
		}
	case Message_File:
		{
			wsprintf(sendBuf,L"%d_%s",Message_File,str.c_str());
			break;
		}
	}
	send(sock,(char*)sendBuf,lstrlen(sendBuf)*2,0);
}

void SyncUserInfo(stUserInfo userinfo)
{
	for (list<stUserInfo>::iterator it = g_listUser.begin(); it != g_listUser.end(); it++)
	{
		if (userinfo.sock == it->sock)
		{
			continue;
		}
		Sleep(20);
		SendToClient(Message_Login,userinfo.sock,it->strName);//把其他的信息同步给我
		Sleep(20);
		SendToClient(Message_Login,it->sock,userinfo.strName);//把我的信息同步给其他人
	}
}

bool GetUserByName(wstring name,stUserInfo& userinfo)
{
	for (list<stUserInfo>::iterator it = g_listUser.begin(); it != g_listUser.end(); it++)
	{
		if (it->strName == name)
		{
			userinfo = *it;
			return true;
		}
	}
	return false;
}

bool GetUserBySocket(SOCKET sock,stUserInfo& userinfo)
{
	for (list<stUserInfo>::iterator it = g_listUser.begin(); it != g_listUser.end(); it++)
	{
		if (it->sock == sock)
		{
			userinfo = *it;
			return true;
		}
	}
	return false;
}

void Chat(SOCKET sockSend,wstring nameSendTo,wstring strText)
{
	//得到被发送者的info
	stUserInfo userSendTo;
	if(!GetUserByName(nameSendTo,userSendTo))
	{
		return;
	}
	//得到发送者的info
	stUserInfo userSend;
	if (!GetUserBySocket(sockSend,userSend))
	{
		return;
	}
	wstring strBuf;
	strBuf = userSend.strName + L"_" +userSendTo.strName + L"_"+strText;
	SendToClient(Message_ChatToMe,userSendTo.sock,strBuf);
	SendToClient(Message_ChatToOther,userSend.sock,strBuf);
}

void SendFile(SOCKET sockSend,wstring nameSendTo,wstring fileName,wstring fileSize)
{
	//得到被发送者的info
	stUserInfo userSendTo;
	if(!GetUserByName(nameSendTo,userSendTo))
	{
		return;
	}
	//得到发送者的info
	stUserInfo userSend;
	if (!GetUserBySocket(sockSend,userSend))
	{
		return;
	}
	wstring strBuf;
	strBuf = userSend.strName + L"_" + fileName + L"_" + fileSize;
	SendToClient(Message_File,userSendTo.sock,strBuf);

	ifstream file(fileName,ios::binary);//二进制打开
	if(!file.is_open())
	{
		cout<<"文件打开失败";
		return ;
	}
	file.seekg(0, ios::beg);

	//开始传送文件
	int nLen=0;//读取长度
	int nSize=0;//发送长度
	
	while(1)//开始传送文件
	{
		char buf[1024*20] = {0};
		//一次读取1024*20大小的文件内容
		file.read(buf, 1024*20);
		nLen = file.gcount();
		if(nLen == 0)
			break;
		nSize = send(userSendTo.sock, (const char *)buf, nLen, 0);
	}

	file.close();
}

void RecvMessage(SOCKET sock,wstring strBuf)
{
	//获得操作类型
	int pos = strBuf.find('_');
	int msgType = Message_Min;
	if (pos != -1)
	{
		wstring bufTemp = strBuf.substr(0,pos);
		strBuf = strBuf.substr(pos+1);
		char* temp = (char*)bufTemp.c_str();
		msgType = atoi(temp);
	}
	else
	{
		char* temp = (char*)strBuf.c_str();
		msgType = atoi(temp);
	}
	if (msgType <= Message_Min || msgType >= Message_Max)
	{
		return;
	}
	switch(msgType)
	{
		case Message_Login:
			{
				//保存信息，包括SOCKET信息和名字
				stUserInfo userInfo;
				userInfo.sock = sock;
				userInfo.strName = strBuf;
				if (IsExist(userInfo.strName))
				{
					userInfo.strName = GetNewName(userInfo);
				}
				g_listUser.push_back(userInfo);
				//同步信息
				SyncUserInfo(userInfo);
				break;
			}
		case Message_Chat:
			{
				//获得发送消息给的人和聊天信息
				pos = strBuf.find('_');
				if (pos == -1)
				{
					return;
				}
				wstring strName = strBuf.substr(0,pos);
				wstring strText = strBuf.substr(pos+1);
				Chat(sock,strName,strText);
				break;
			}
		case Message_File:
			{
				pos = strBuf.find('_');
				if (pos == -1)
				{
					return;
				}
				wstring strName = strBuf.substr(0,pos);//接受者名字
				strBuf = strBuf.substr(pos+1);
				pos = strBuf.find('_');
				if (pos == -1)
				{
					return;
				}
				wstring strFileName = strBuf.substr(0,pos);//文件名字
				string c_strFileName = string(strFileName.begin(),strFileName.end());
				wstring strFileSize = strBuf.substr(pos+1);//文件大小

				int dwCount = 0;
				ofstream file(c_strFileName.c_str(),ios::binary);//在服务端当前目录创建接收到的文件
				//阻塞 接受文件
				while (true)
				{
					char buf[1024*20] = {0};
					int length = recv(sock, (char*)buf, 1024*20, 0);
					if(length==0)
						break;

					//将接收到的文件写到新建的文件中去
					file.write(buf, length);
					dwCount += length;

					if(dwCount == _wtoi(strFileSize.c_str()))
						break;
				}

				file.close();//关闭文件 服务端接受完毕
				SendFile(sock,strName,strFileName,strFileSize);//转发给目标客户端
				break;
			}
	}
}

void LoginOut(SOCKET sock)
{
	for (list<stUserInfo>::iterator it = g_listUser.begin(); it != g_listUser.end(); ++it)
	{
		if (it->sock == sock)
		{
			//把消息发送给其他人，告诉下线了
			for (list<stUserInfo>::iterator it2 = g_listUser.begin(); it2 != g_listUser.end(); ++it2)
			{
				if (it2->sock != it->sock)
				{
					SendToClient(Message_LoginOut,it2->sock,it->strName);
				}
			}
			g_listUser.erase(it);
			break;
		}
	}
}

//接收数据的线程
void receive(PVOID param)
{
	SOCKET sock = *((SOCKET*)param);
	char buf[2048] = {0};
	int bytes;
	while(1)
	{
		//接收数据
		if ((bytes = recv(sock,buf,sizeof(buf),0))== SOCKET_ERROR)
		{
			LoginOut(sock);//下线
			_endthread();//关闭线程
			return;
		}
		buf[bytes] = '\0';
		wchar_t bufTest[1024] = {0};
		memcpy(bufTest,buf,bytes);
		bufTest[bytes/2] = '\0';
		RecvMessage(sock,bufTest);
	}
}

void ReceiveConnectThread(void* param)
{
	SOCKET socketServer = *((SOCKET*)param);
	while(1)
	{
		SOCKET clientSocket;	//用来接收客户端连接
		struct sockaddr_in clientAddress;	//套接的地址，端口
		memset(&clientAddress,0,sizeof(clientAddress));
		int addrLen = sizeof(clientAddress);
		if (INVALID_SOCKET ==(clientSocket = accept(socketServer,(sockaddr*)&clientAddress,&addrLen)))
		{
			cout << "接受客户端连接失败";
			return ;
		}

		_beginthread(receive,0,&clientSocket);//如果客户端成功接入，开启处理该客户端线程，每来一个开一条

	}
}

SOCKET StartServer()
{
	SOCKET serverSocket;
	if ((serverSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) == INVALID_SOCKET)
	{
		cout << "创建套接字失败";
		return -1;
	}

	struct sockaddr_in serverAddress;
	memset(&serverAddress,0,sizeof(sockaddr_in));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//127.0.0.1
	serverAddress.sin_port = htons(1997);

	//绑定
	if (bind(serverSocket,(sockaddr*)&serverAddress,sizeof(serverAddress))== SOCKET_ERROR)
	{
		cout << "套接字绑定失败" << endl;
		return 0;
	}

	//监听
	if (listen(serverSocket,SOMAXCONN) == SOCKET_ERROR)
	{
		cout << "监听失败";
		return 0;
	}

	return serverSocket;
}

int _tmain(int argc, _TCHAR* argv[])
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
	{
		cout << "套接字初始化失败";
		return -1;
	}

	SOCKET sock = StartServer();//启动服务器

	_beginthread(ReceiveConnectThread,0,&sock);//开启接受客户端线程

	char buf[1024];
	while(1)
	{
		gets_s(buf);
	}
	return 0;
}

