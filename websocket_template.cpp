/*
* Template for creating your own websocket program that can send json messages to a websocket server, ex. NI Mate
* 
 
* The program works in the following way:
- When the program is launched, you are prompted to input the IP address of the Websocket server
- After a connection to the server has been made, the program will wait for a Start message (if that is what you want)
- Once the start message has been recieved, the program will start to send data
-A pause message can be sent to the client from the server if you want to pause sending data

* Things to note: 
	
*/

///General Header files///
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <new>
#include <math.h>
#include <stdint.h>

///Websocket++ Header files///
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

///Boost Header files///
#include <boost/thread/thread.hpp>

///JSON Header files///
#include "json.hpp"
using json = nlohmann::json;

///Define///
#define MAX_RETRIES 5		//Nr of time the program will try to reconnect to the websocket server before closing
#define WAIT_TIME 8			//Define wait time for limiting the nunber of messages sent per second

///Websocket++ typedef///
typedef websocketpp::client<websocketpp::config::asio_client> client;

///Websocket++ Variable///
bool disconnected = true;	//If we are disconnected from server
int retries = 0;			//If connection to server is lost we save the number of retries we do, exit program after x retries
bool start = false;			//Value that is checked before we start to send data
bool stop = false;			//Value that is checked each iteration, true if we want to stop the client
bool paused = false;		//Value that is checked each iteratino, true if we want to pause the data being sent

///Time varibles///
boost::posix_time::ptime time_start;		//When to start couting toeards the wait time
boost::posix_time::ptime time_end;			//Used to compare/determine how long time has passed
boost::posix_time::time_duration diff;		//The time difference betweene time_start and time_end


///Classes for websocket communication///
/*
* We use Websocket++ (Websocketpp) to connect to a websocket server
* Classes are from the Websocket Utils example but have been modified to our own needs
*/

class connection_metadata {

public:
    typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;

    connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri)
      : m_id(id)
      , m_hdl(hdl)
      , m_status("Connecting")
      , m_uri(uri)
      , m_server("N/A")
    {}

    void on_open(client * c, websocketpp::connection_hdl hdl) {
        m_status = "Open";

        client::connection_ptr con = c->get_con_from_hdl(hdl);
        m_server = con->get_response_header("Server");
    }

    void on_fail(client * c, websocketpp::connection_hdl hdl) {
        m_status = "Failed";

        client::connection_ptr con = c->get_con_from_hdl(hdl);
        m_server = con->get_response_header("Server");
        m_error_reason = con->get_ec().message();
    }

	void on_close(client * c, websocketpp::connection_hdl hdl) {
    m_status = "Closed";
    client::connection_ptr con = c->get_con_from_hdl(hdl);
    std::stringstream s;
    s << "close code: " << con->get_remote_close_code() << " (" 
      << websocketpp::close::status::get_string(con->get_remote_close_code()) 
      << "), close reason: " << con->get_remote_close_reason();
    m_error_reason = s.str();
	}

	void on_message(websocketpp::connection_hdl, client::message_ptr msg) {		//Class that handles recieved messages
		//std::cout << msg->get_payload().c_str() << std::endl;					//Use if you need to debug received messages
        if (msg->get_opcode() == websocketpp::frame::opcode::text) {
			std::string incoming_msg = msg->get_payload(); 						//Convert to string
			json json_msg = json::parse(incoming_msg);							//Parse to Json
			
			// Example on how to use/analyze a Json message that has been recieved //

			if (json_msg.find("type") != json_msg.end()) {						//The start/pause/stop message key name is "type" with the value "start" / "pause" / "stop"
				std::string type_cmd = json_msg["type"].get<std::string>();
				if (type_cmd == "start") { 										//Start message from the server which will be sent to the client when the server is ready to receive data
					start = true;												//We set start to true to start polling and sending data
					paused = false;
				}
				else if (type_cmd == "quit") { 									//Stop message from the server which will be sent to the client when the server wants to stop the client
					stop = true;												//We set stop to true to stop the program
				}
				else if (type_cmd == "stop") { 									//Stop message from the server which will be sent to the client when the server wants to pause the client sending data
					paused = !paused;											//Flip the value of pause to either pause or unpause our program
				}
			}
			incoming_msg.clear();												//Clear the strings just in case
			json_msg.clear();													//Clear the Json message just in case
        }
		else {
            //m_messages.push_back("<< " + websocketpp::utility::to_hex(msg->get_payload())); //If the message is not a string or similar, we only send JSON/Strings
        }
    }
    websocketpp::connection_hdl get_hdl() const {
        return m_hdl;
    }
    int get_id() const {
        return m_id;
    }
    std::string get_status() const {
        return m_status;
    }
    void record_sent_message(std::string message) {
        //m_messages.push_back(">> " + message);	//We don't need to record messages, use if you want but watch out for memory leaks
    }
    friend std::ostream & operator<< (std::ostream & out, connection_metadata const & data);

private:
    int m_id;
    websocketpp::connection_hdl m_hdl;
    std::string m_status;
    std::string m_uri;
    std::string m_server;
    std::string m_error_reason;
    std::vector<std::string> m_messages;
};

std::ostream & operator<< (std::ostream & out, connection_metadata const & data) {
    out << "> URI: " << data.m_uri << "\n"
        << "> Status: " << data.m_status << "\n"
        << "> Remote Server: " << (data.m_server.empty() ? "None Specified" : data.m_server) << "\n"
        << "> Error/close reason: " << (data.m_error_reason.empty() ? "N/A" : data.m_error_reason) << "\n";
    out << "> Messages Processed: (" << data.m_messages.size() << ") \n";

    std::vector<std::string>::const_iterator it;
    for (it = data.m_messages.begin(); it != data.m_messages.end(); ++it) {
        out << *it << "\n";
    }
    return out;
}

class websocket_endpoint {

public:
    websocket_endpoint () : m_next_id(0) {
        m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
        m_endpoint.clear_error_channels(websocketpp::log::elevel::all);
        m_endpoint.init_asio();
        m_endpoint.start_perpetual();
		m_thread = websocketpp::lib::make_shared<websocketpp::lib::thread>(&client::run, &m_endpoint);
        //m_thread.reset(new websocketpp::lib::thread(&client::run, &m_endpoint));
    }

	~websocket_endpoint() {
		m_endpoint.stop_perpetual();

		for (con_list::const_iterator it = m_connection_list.begin(); it != m_connection_list.end(); ++it) {
			if (it->second->get_status() != "Open") {
				// Only close open connections
				continue;
			}
			std::cout << "> Closing connection " << it->second->get_id() << std::endl;
			websocketpp::lib::error_code ec;
			m_endpoint.close(it->second->get_hdl(), websocketpp::close::status::going_away, "", ec);
			if (ec) {
				std::cout << "> Error closing connection " << it->second->get_id() << ": "  
						<< ec.message() << std::endl;
			}
		}
	m_thread->join();
	}

    int connect(std::string const & uri) {
        websocketpp::lib::error_code ec;

        client::connection_ptr con = m_endpoint.get_connection(uri, ec);
        if (ec) {
            std::cout << "> Connect initialization error: " << ec.message() << std::endl;
            return -1;
        }
        int new_id = m_next_id++;
        connection_metadata::ptr metadata_ptr = websocketpp::lib::make_shared<connection_metadata>(new_id, con->get_handle(), uri);
        m_connection_list[new_id] = metadata_ptr;

        con->set_open_handler(websocketpp::lib::bind(
            &connection_metadata::on_open,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1
        ));
        con->set_fail_handler(websocketpp::lib::bind(
            &connection_metadata::on_fail,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1
        ));
        con->set_close_handler(websocketpp::lib::bind(
            &connection_metadata::on_close,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1
        ));
        con->set_message_handler(websocketpp::lib::bind(
            &connection_metadata::on_message,
            metadata_ptr,
            websocketpp::lib::placeholders::_1,
            websocketpp::lib::placeholders::_2
        ));

        m_endpoint.connect(con);

        return new_id;
    }

	void send(int id, std::string message) {
		websocketpp::lib::error_code ec;

		con_list::iterator metadata_it = m_connection_list.find(id);

		if (metadata_it == m_connection_list.end()) {
			std::cout << "> No connection found with id " << id << std::endl;
			return;
		}

		m_endpoint.send(metadata_it->second->get_hdl(), message, websocketpp::frame::opcode::text, ec);

		if (ec) {
			std::cout << "> Error sending message: " << ec.message() << std::endl;
			std::cout << "> Trying again after 5 seconds \n";
			boost::this_thread::sleep(boost::posix_time::seconds(5)); 	//If a send fails the server has most likely crashed/stopped, wait 5 seconds if this happens then try to reconnect
			retries++;													//Keep track of the number of retries
			return;
		}
		retries = 0;
	}

	void close(int id, websocketpp::close::status::value code, std::string reason) {
        websocketpp::lib::error_code ec;

        con_list::iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end()) {
            std::cout << "> No connection found with id " << id << std::endl;
            return;
        }

        m_endpoint.close(metadata_it->second->get_hdl(), code, reason, ec);
        if (ec) {
            std::cout << "> Error initiating close: " << ec.message() << std::endl;
        }
    }

	connection_metadata::ptr get_metadata(int id) const {
		con_list::const_iterator metadata_it = m_connection_list.find(id);
		if (metadata_it == m_connection_list.end()) {
			return connection_metadata::ptr();
		} else {
			return metadata_it->second;
		}
	}

private:
    typedef std::map<int,connection_metadata::ptr> con_list;

    client m_endpoint;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;

    con_list m_connection_list;
    int m_next_id;
};

int main(int argc, char *argv[]) {

	///Websocket++ Setup///
	std::string ip_address;
	int id = -1;
	websocket_endpoint endpoint;
	
	while(id < 0) {
		std::cout << "Enter IP address and port, example: ws://192.168.2.4:7681 \n";
		std::cin >> ip_address;
		id = endpoint.connect(ip_address); 						//This will warn if the IP address is not correct
	}

	boost::this_thread::sleep(boost::posix_time::seconds(2));	//Wait for connection

	///Initial contact with websocket server///

	///Initial JSON message template that will be sent to the Server, Just as an example///
	char Json[] = R"({
		"type": "device",
		"value": {
			"device_id": "",
			"device_type": "psmove",
			"name": "psmove",
			"values": [ {
				"name": "controller id",
				"type": "array",
				"datatype": "string",
				"count": ,
				"ID": []
			}
			],
		}	
	})";

	std::string init_msg(Json);					//Create string from the above char message

	//std::cout << init_msg << std::endl;		//In case of bug

	json j_complete = json::parse(init_msg);	//Parse our message to Json

	std::string start_message;
	start_message = j_complete.dump();			//Dump the Json back to string so that it can be sent to the server

	endpoint.send(id, start_message);			//Send the initial message	

	///Main loop, polls data and sends it to websocket server///

	while(!start) {								//Wait for server to send start command, remove if you want to send data without waiting for a start message
		boost::this_thread::sleep(boost::posix_time::seconds(1));	
	}

	while (retries < MAX_RETRIES ) {			//Close after 5 retries 

		time_start = boost::posix_time::microsec_clock::local_time();
		time_end = boost::posix_time::microsec_clock::local_time();
		diff = time_end - time_start;

		while (diff.total_milliseconds() < WAIT_TIME ) {			//Limit the data sending rate, Change according to own needs
			usleep(1);
			time_end = boost::posix_time::microsec_clock::local_time();
			diff = time_end - time_start;
		}

			json_j["type"] = "data";								//Example on how a Json message can be created
			json_j["value"]["timestamp_ms"] = (time(0)*1000);

			while ((retries > 0) && (retries < MAX_RETRIES)) {
				int close_code = websocketpp::close::status::service_restart;	//Reason why we are closing connection
				std::string reason = "Trying to re-connect";
				endpoint.close(id, close_code, reason);							//This will fail if server was closed, might be useless
				id = endpoint.connect(ip_address);								//Try to reconnect to the server, id will be old id + 1
				boost::this_thread::sleep(boost::posix_time::seconds(2)); 		//Have to wait a couple of seconds to reconnect
				endpoint.send(id, start_message);
			}
			
			message = json_j.dump();											//Dump the Json message to a string so it can be sent
			endpoint.send(id, message);											//Send the message / data to the server
			//std::cout << message << std::endl;								//In case we need to debug the message that is sent

			///Cleanup before next iteration, avoid memory leaks///
			message.clear();
			json_j.clear();
		}

		if (paused) {
			boost::this_thread::sleep(boost::posix_time::milliseconds(500)); 		//Wait 500ms then check if we should unpause
		}

		if (stop) {
			break;
		}
	}

	///Websocket Cleanup///
	int close_code = websocketpp::close::status::normal;
    std::string reason = "Quit program";
    endpoint.close(id, close_code, reason);	//Close the connection with the websocket server
	std::cout << "> Program has closed \n";
	return 0;    
}