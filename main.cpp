#include <dpp/dpp.h>

#include <chrono>
#include <thread>
#include <future>
#include <vector>

using std::string;
using std::vector;
using std::stringstream;

const string BOT_TOKEN = "";
const string BOT_NAME = "";
const dpp::snowflake channel_id(1040068528885223558);
const bool debug = false;

vector<dpp::message*> messages;

// util functions
void debug_print(string msg) {
    std::cout << msg << std::endl;
}

void error_print(string msg) {
    std::cout << "Error: " << msg << std::endl;
}

// actual functions

// prints out messages to console
void messagePrint(string user, string content, time_t msg_time, int idx) {
    struct tm * timeinf;
    timeinf = localtime(&msg_time);

    string local_time = asctime(timeinf);
    local_time[local_time.size()-1] = '\0';

    std::cout << "#" << idx << " (" << local_time << ") " << user << ": " << content << std::endl;
}

// loops through message vector and prints all messages
void print_messages() {
    if (messages.size() > 0) {
        // clear cmd
        system("clear");

        for (int i = 0; i < messages.size(); i++) {
            string msg_content = messages[i]->content;
            string msg_sender = messages[i]->author.username;
            time_t msg_time = messages[i]->sent;
            messagePrint(msg_sender, msg_content, msg_time, i);
        }
    }
}

/*
    edits message sent by user

    return 0: success
    return -1: invalid msg_idx

*/
int edit_message(string msg_content, string smsg_idx, dpp::cluster* bot) {
    int msg_idx = atoi(smsg_idx.c_str());

    if (msg_idx > 99) {
        return -1;
    } else if (messages[msg_idx] == nullptr) {
        return -1;
    } else if (messages[msg_idx]->author.username != BOT_NAME) {
        return -1;
    }

    messages[msg_idx]->content = msg_content;
    bot->message_edit_sync(*messages[msg_idx]);
    print_messages();

    return 0;
}

/*
    deletes message sent by user

    return 0: success
    return -1: invalid msg_idx

*/
int delete_message(string smsg_idx, dpp::cluster* bot) {
    int msg_idx = atoi(smsg_idx.c_str());

    if (msg_idx > 99) {
        return -1;
    } else if (messages[msg_idx] == nullptr) {
        return -1;
    } else if (messages[msg_idx]->author.username != BOT_NAME) {
        return -1;
    }

    dpp::snowflake msg_id = messages[msg_idx]->id;
    bot->message_delete_sync(msg_id, channel_id);
    
    delete messages[msg_idx];
    messages.erase(messages.begin()+msg_idx);
    print_messages();

    return 0;
}

// https://thispointer.com/c11-multithreading-part-9-stdasync-tutorial-example/
void get_input(dpp::cluster* bot) {
    std::string input = "";

    while (true) {
        // waits 1 sec for input
        std::this_thread::sleep_for(std::chrono::seconds(1));
        getline(std::cin, input);
        stringstream ss(input);

        if (!input.empty()) {
            string first_word;
            ss >> first_word;

            if (first_word == ".exit") {
                exit(0);
            } else if (first_word == ".edit") {
                string msg_num;
                ss >> msg_num;

                if (msg_num.empty()) {
                    error_print("Missing message number to edit.");
                }

                string msg_content;
                while (!ss.eof()) {
                    string tmp;
                    ss >> tmp;

                    msg_content += tmp;
                    msg_content += " ";
                }

                if (msg_content.empty()) {
                    error_print("Missing edited message content.");
                }

                int ret = edit_message(msg_content, msg_num, bot);

                if (ret == -1) {
                    error_print("Invalid message index.");
                }

                continue;
            } else if (first_word == ".delete") {
                string msg_num;
                ss >> msg_num;

                if (msg_num.empty()) {
                    error_print("Missing message number to edit.");
                }

                int ret = delete_message(msg_num, bot);

                if (ret == -1) {
                    error_print("Invalid message index.");
                }
                continue;
            }
            
            else {
                // create message object, using channel_id and inputted string
                dpp::message msg(channel_id, input);
                bot->message_create(msg);
            }
        }
    }
}

int main() {
    // creates bot
    dpp::cluster bot(BOT_TOKEN, dpp::i_default_intents | dpp::i_message_content);
    dpp::cluster* bot_ptr = &bot;

    // async input gathering
    std::future<void> input = std::async(std::launch::async, [&]{get_input(bot_ptr);});

    // debug util
    if (debug) {
        bot.on_log(dpp::utility::cout_logger());
    }

    // listens for messages being sent, and prints
    bot.on_message_create([&](const dpp::message_create_t& event) {
        dpp::message* m = new dpp::message();
        *m = event.msg;
        messages.push_back(m);
        print_messages();
    });

    // listens for messages being updated, changes information in the vector, then prints
    bot.on_message_update([&](const dpp::message_update_t& event) {
        dpp::message* m = new dpp::message();
        *m = event.msg;

        for (int i = 0; i < messages.size(); i++) {
            if (messages[i]->id == m->id) {
                messages[i] = m;
                break;
            }
        }

        print_messages();
    });

    // on message delete
    bot.on_message_delete([&](const dpp::message_delete_t& event) {
        for (int i = 0; i < messages.size(); i++) {
            if (messages[i]->id == event.deleted->id) {

                delete messages[i];
                messages.erase(messages.begin() + i);
                break;
            }
        }

        print_messages();
    });

    // on load
    bot.on_ready([&bot](const dpp::ready_t& event) {
        system("clear");
        // unordered_map of dpp::messages 
        dpp::message_map msgs = bot.messages_get_sync(channel_id, 0, 0, 0, 100);

        for (auto i : msgs) {
            dpp::message* m = new dpp::message();
            *m = i.second;
            messages.push_back(m);
        }

        print_messages();
    });

    bot.start(dpp::st_wait);
    return 0;
}
