#include "msgsock.h"
#include "css.h"
#include "css_test.h"

#pragma comment(lib, "uv\\libuv.lib")

#define CHECKGOTO(X) if((X) < 0){goto _clean;}

static void do_nothing(uv_work_t* req)
{
    ::Sleep(3000);
}

static void reconn(uv_work_t* req, int status)
{
    msgsock* s = (msgsock*)req->data;
    printf("recconn s stat: %d\n", s->get_stat());
    int ret = s->connect_sever("127.0.0.1", 5566);
    printf("reconn return %d\n", ret);
    free(req);
}

static void do_newuser(MyMSG* msg, msgsock* s)
{
    printf("received new user: id: %d; room: %d\n", msg->body.userinfo.userid, msg->body.userinfo.roomid);
    s->dis_connect();
    uv_work_t* req = (uv_work_t*)malloc(sizeof(uv_work_t));
    req->data = s;
    int ret = uv_queue_work(uv_default_loop(), req, do_nothing, reconn);
    printf("queue reconn return %d\n", ret);
}

static void do_userlogout(MyMSG* msg, msgsock* s)
{
    printf("received user %d logout\n", msg->body.userinfo.userid);
}

static void do_socket_error(MyMSG* msg, msgsock* s)
{
    printf("received socket error\n");
}

static void on_msg(MyMSG* msg, void* userdata)
{
    msgsock* s = (msgsock*)userdata;
    switch (msg->code){
    case CODE_NEWUSER:
        do_newuser(msg, s);
        break;
    case CODE_USERLOGOUT:
        do_userlogout(msg, s);
        break;
    case CODE_SOCKETERROR:
        do_socket_error(msg, s);
        break;
    default:
        break;
    }
}

static void on_frame(cc_src_sample_t* frame, void* userdata)
{}

static void connect_cb(uv_connect_t* req, int status)
{
    printf("!!connect_cb status: %d\n", status);
    uv_close((uv_handle_t*)req->handle, NULL);
}

TEST_IMPL(msgsock)
{
    int ret = -1;
	uv_loop_t* loop = uv_default_loop();
    if (!loop) return;
    cc_userinfo_t userinfo = { 0 };
    cc_userinfo_t userinfo2 = { 0 };

    userinfo.port = 5566;
    userinfo.roomid = 24;
    userinfo.userid = 103;

    userinfo2.port = 5566;
    userinfo2.roomid = 24;
    userinfo2.userid = 104;

    CHECKGOTO(ret = css_server_init(NULL, 0));
    CHECKGOTO(ret = css_server_start());

    //{
    //    int ret = 0;
    //    uv_tcp_t *tcp_stream = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
    //    uv_tcp_init(loop, tcp_stream);
    //    uv_connect_t *connect_req;
    //    connect_req = (uv_connect_t*)malloc(sizeof(uv_connect_t));
    //    sockaddr_in addr;
    //    uv_ip4_addr("192.168.1.1", 80, &addr);
    //    ret = uv_tcp_connect(connect_req, tcp_stream, (const struct sockaddr*)&addr, connect_cb);
    //    if (ret){
    //        free(connect_req);
    //    }
    //}

    msgsock* s = new msgsock(loop, NULL);
    s->set_local_user(&userinfo);
    s->on_received(s, on_msg);
    s->on_received_frame(s, on_frame);

    msgsock* s2 = new msgsock(loop, NULL);
    s2->set_local_user(&userinfo2);
    s2->on_received(s2, on_msg);
    s2->on_received_frame(s2, on_frame);

    ret = s->connect_sever("192.168.1.101", 5566);
    printf("connect_sever return %d\n", ret);
    ret = s2->connect_sever("127.0.0.1", 5566);
    printf("connect_sever return %d\n", ret);
    printf("s stat: %d\n", s->get_stat());
    printf("s2 stat: %d\n", s2->get_stat());
    uv_run(loop, UV_RUN_DEFAULT);
    printf("stop!\n");
_clean:
    css_server_stop();
    css_server_clean();
    delete s;
    return;
}