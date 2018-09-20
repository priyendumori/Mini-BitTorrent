/*============================================================
    @author - Rushitkumar Jasani   @rollno - 2018201034
=============================================================*/
/*
 * ./tracker <my_tracker_ip>:<my_tracker_port> <other_tracker_ip>:<other_tracker_port> <seederlist_file> <log_file>
 * ./tracker 10.1.38.138:6565 10.1.38.138:6567 seed_file log_file
 * ./tracker 127.0.0.1:6565 127.0.0.1:6567 seed_file log_file
 */

#ifndef CL_HEADER_H
#define CL_HEADER_H
#include "server_header.h"
#endif

#ifndef CL_GLOBAL_H
#define CL_GLOBAL_H
#include "server_global.h"
#endif

void serve(int cl_soc)
{
    char buffer[1024] = {0};
    read(cl_soc, buffer, 1024);

    vector<string> client_req;
    char *token = strtok(buffer, SEP.c_str());
    while (token)
    {
        client_req.push_back(token);
        token = strtok(NULL, SEP.c_str());
    }
    string key_hash = client_req[1];
    string cl_socket = client_req[2];
    string file_path = client_req[3];
    string buf_buf = key_hash + SEP + cl_socket + SEP + file_path;
    cout << buf_buf << endl;
    if (client_req[0] == "0")
    {
        //share
        if (seeder_map.find(key_hash) == seeder_map.end())
        {
            seeder_map[key_hash][cl_socket] = file_path;
            append_to_seederlist(buf_buf);
        }
        else
        {
            if (seeder_map[key_hash].find(cl_socket) == seeder_map[key_hash].end())
            {
                seeder_map[key_hash][cl_socket] = file_path;
                append_to_seederlist(buf_buf);
            }
        }
    }
    else if (client_req[0] == "1")
    {
        //remove
        if (seeder_map.find(key_hash) != seeder_map.end())
        {
            // cout << "data exist" << endl;
            map<string, string> temp;
            temp.insert(seeder_map[key_hash].begin(), seeder_map[key_hash].end());
            if (temp.size() == 1 && temp.find(cl_socket) != temp.end())
            {
                seeder_map.erase(key_hash);
                write_to_seederlist();
            }
            else if (temp.size() != 1)
            {
                seeder_map[key_hash].erase(cl_socket);
                write_to_seederlist();
            }
        }
    }
    else if (client_req[0] == "2")
    {
        //get
        string res;
        for (auto i : seeder_map[key_hash])
        {
            res = res + i.first + SEP + i.second + SEP;
        }
        cout << res << endl;
        string sz = to_string(res.size());
        send(cl_soc, sz.c_str(), sz.size(), 0);
        send(cl_soc, res.c_str(), res.size(), 0);
        cout << "Data sent to client!  :) ";
    }
    else if (client_req[0] == "3")
    {
        //EXIT
        map<string, map<string, string>>::iterator i;
        vector<map<string, map<string, string>>::iterator> del;
        for (i = seeder_map.begin(); i != seeder_map.end(); ++i)
        {
            if (i->second.find(cl_socket) != i->second.end())
            {
                i->second.erase(cl_socket);
            }
            if (i->second.size() == 0)
            {
                del.push_back(i);
            }
        }
        for (auto i : del)
        {
            seeder_map.erase(i);
        }
        write_to_seederlist();
    }
    cout << "REQUEST " << client_req[0] << " SERVED" << endl;
    print_map();
    close(cl_soc);
    return;
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        cout << "improper arguments :( \n"
             << endl;
        exit(1);
    }
    else
    {
        process_args(argv);
        writeLog("Argument Processed.");
        read_seederlist();
        writeLog("SeededList File Read Successfully.");
        struct sockaddr_in tr1_addr;
        int sock = 0;
        int opt = 1;
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        {
            writeLog("Socket creation error");
            exit(EXIT_FAILURE);
        }
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
        {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }
        tr1_addr.sin_family = AF_INET;
        tr1_addr.sin_port = htons(tr1_port);
        tr1_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(sock, (struct sockaddr *)&tr1_addr, sizeof(tr1_addr)) < 0)
        {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }
        writeLog("Bind Successful.");
        if (listen(sock, 5) < 0)
        {
            perror("listen");
            exit(EXIT_FAILURE);
        }
        writeLog("Socket Created with sock = "+to_string(sock)+"." );
        writeLog("Listening for clients.");
        int cl_soc;
        int addrlen = sizeof(tr1_addr);
        while (1)
        {
            cl_soc = accept(sock, (struct sockaddr *)&tr1_addr, (socklen_t *)&addrlen);
            if (cl_soc < 0)
            {
                perror("IN ACCEPT : ");
                continue;
            }
            writeLog("Connection accepted.");
            try
            {
                writeLog("Thread Created.");
                std::thread t1(serve, std::ref(cl_soc));
                t1.detach();
            }
            catch (const std::exception &ex)
            {
                std::cout << "Thread exited with exception: " << ex.what() << "\n";
            }
        }
    }
    return 0;
}