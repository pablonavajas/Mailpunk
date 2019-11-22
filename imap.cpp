//Imperial College London - Department of Computing
//MSc Computing CO157 Object-Oriented Design & Programming
//Assessed Exercise No. 1
//Mailpunk
//
//Created by: Pablo Navajas Helguero
//Date: 20th November 2019
//
//File: imap.cpp


#include "imap.hpp"
#include <sstream>

using namespace std;
using namespace IMAP;

/**
 * In-line constructor definition for updateUI function
 */
Message::Message(Session* Session, int UID): session(Session) {
  /**
   * Initialise Mesage data members in constructor
   */
  msg_cont = "";
  uid = UID;

  session = Session; 
}


void Message::getAtts(clist* contents) {

  /**
   * For each attribute in the clist, check if it is 
   * 
   * the body, the subject or the "from" field
   */
  auto msg_att = (struct mailimap_msg_att*)clist_content(clist_begin(contents));
  
  for (clistiter* attidx = clist_begin(msg_att->att_list) ; attidx != nullptr ; attidx = clist_next(attidx) ) {

    auto item = (struct mailimap_msg_att_item*) clist_content(attidx);

    if (item->att_type == MAILIMAP_MSG_ATT_ITEM_STATIC) {

      if (item->att_data.att_static->att_type == MAILIMAP_MSG_ATT_BODY_SECTION) {
	
	msg_cont = item->att_data.att_static->att_data.att_body_section->sec_body_part;
      }
    }
  }   
}


std::string Message::getBody(){
  /**
   * Create set to fetch a single message, and fetch structure
   * 
   * to retrieve only the body of the message
   */ 
  mailimap_set* set = mailimap_set_new_single(uid);
  
  struct mailimap_fetch_type* fetchStruct = mailimap_fetch_type_new_fetch_att_list_empty();

  clist * contents;

  struct mailimap_section * bodySection = mailimap_section_new(nullptr);
  
  auto body_att = mailimap_fetch_att_new_body_section(bodySection);

  /**
   * Add body attribute to fetching list and fetch
   */
  mailimap_fetch_type_new_fetch_att_list_add(fetchStruct, body_att);

  std::string ErrMsg = "Unable to retrieve message";
  
  check_error( mailimap_uid_fetch(session->imap, set, fetchStruct, &contents), ErrMsg);

  getAtts(contents);
  
  
  /**
   * Deallocate the necessary memory
   */
  mailimap_fetch_list_free(contents);
  
  mailimap_fetch_type_free(fetchStruct);

  mailimap_set_free(set);

  
  if (msg_cont.empty())
    return "<>";
  
  return msg_cont;
}


void Message::extractfield(const std::string &field) {
  /**
   * Extract field content up to carriage return character
   */
  unsigned first = msg_cont.find(field + ':');

  unsigned pos_delim = first + field.length() + 2;

  unsigned last = msg_cont.find_first_of('\r', pos_delim);

  msg_cont = msg_cont.substr(pos_delim, last - pos_delim);
}


std::string Message::getField(std::string fieldname){
  /**
   * Create a clist holding the fieldname, a list of headers, and 
   * 
   * a header section structure to define the header attribute to be fetched
   */  
  struct mailimap_section* headSection = mailimap_section_new_header();

  mailimap_fetch_att* head_att = mailimap_fetch_att_new_body_section(headSection);


  /**
   * Create set to fetch a single message, and fetch structure
   * 
   * to retrieve only the specific header of the message
   */ 
  mailimap_set* set = mailimap_set_new_single(uid);
  
  struct mailimap_fetch_type* fetchStruct = mailimap_fetch_type_new_fetch_att_list_empty();

  clist * contents;


  mailimap_fetch_type_new_fetch_att_list_add(fetchStruct, head_att);

  std::string ErrMsg = "Unable to retrieve header";
  
  check_error( mailimap_uid_fetch(session->imap, set, fetchStruct, &contents), ErrMsg);

  getAtts(contents);
  
  /**
   * Extract field content
   */
  extractfield(fieldname);

  /**
   * Deallocate the necessary memory
   */
  mailimap_fetch_list_free(contents);
  
  mailimap_fetch_type_free(fetchStruct);
  
  mailimap_set_free(set);

  
  if (msg_cont.empty())
    return "<>";
  
  return msg_cont;
}


void Message::deleteFromMailbox(){
  
  /**
   * Create set to fetch a single message, list of flags and \Deleted flag object
   */
  struct mailimap_set* set = mailimap_set_new_single(uid);

  struct mailimap_flag_list* DelFlagList = mailimap_flag_list_new_empty();

  struct mailimap_flag* DelFlag = mailimap_flag_new_deleted();
  
  std::string ErrMsg = "Error deleting message";
  
  check_error( mailimap_flag_list_add(DelFlagList, DelFlag), ErrMsg);
  
  /**
   * Change message flags (mark as \Deleted) and remove from mailbox
   */
  
  struct mailimap_store_att_flags* store_att = mailimap_store_att_flags_new_set_flags(DelFlagList);

  check_error( mailimap_uid_store(session->imap, set, store_att), ErrMsg);

  check_error( mailimap_expunge(session->imap), ErrMsg);
  
  /**
   * Deallocate the necessary memory
   */
  mailimap_store_att_flags_free(store_att);
  
  mailimap_set_free(set);

  session->deleteExcept(uid);
  /**
   * Call for UI update
   */
  session->updateUI();
  
  delete this;
}



/**
 * In-line constructor definition for updateUI function
 */
Session::Session(std::function<void()> updateUI): updateUI(updateUI)
{
  /**
   * Create new IMAP session and initialise data members
   */
  imap = mailimap_new(0,nullptr);
  messages = nullptr;
  NoMessages = 0;
  mailbox = "";
}


Session::~Session(){
  
  /**
   * Free structures from IMAP session and logout
   */
  deleteExcept(-1);
  
  mailimap_logout(imap);
  mailimap_free(imap);
}


void Session::CountMessages() {

  /**
   * Create list of status requests, add the number of messages
   *
   * and return information of mailbox
   */
  struct mailimap_status_att_list* StatusAttList = mailimap_status_att_list_new_empty();

  mailimap_status_att_list_add(StatusAttList, MAILIMAP_STATUS_ATT_MESSAGES);

  mailimap_mailbox_data_status* StatusData;

  
  std::string ErrMsg = "Unable to obtain No of Messages";

  check_error(mailimap_status(imap, mailbox.c_str(), StatusAttList, &StatusData), ErrMsg);

  NoMessages =((struct mailimap_status_info*) clist_content(clist_begin(StatusData->st_info_list)))->st_value;

  /**
   * Deallocate the necessary memory
   */
  mailimap_mailbox_data_status_free(StatusData);
  mailimap_status_att_list_free(StatusAttList);
}


int Session::getUID(mailimap_msg_att* msg_att) {

  /**
   * For each attribute in the list, check if it is the UID
   */
  for ( clistiter* att_iter = clist_begin(msg_att->att_list); att_iter != nullptr; att_iter = clist_next(att_iter) ) {

    auto item = (struct mailimap_msg_att_item*) clist_content(att_iter);

    if ( item->att_type == MAILIMAP_MSG_ATT_ITEM_STATIC and item->att_data.att_static->att_type == MAILIMAP_MSG_ATT_UID )
      return item->att_data.att_static->att_data.att_uid;

    return 0;
  }
}

void Session::deleteExcept(int msg_uid) {

  for (int i = 0 ; i < NoMessages ; i++) {

    if (messages[i]->uid != msg_uid) {
      
      delete messages[i];
    }
  }

  delete [] messages;
}

Message** Session::getMessages(){

  /**
   * Initialise the required number of message instances
   */
  CountMessages();

  if (!NoMessages)
    return new Message*[1]{};

  messages = new Message*[NoMessages + 1];
  
  /**
   * Create an interval 1:* * to fetch all messages 
   */
  struct mailimap_set* set = mailimap_set_new_interval(1,0);

  /**
   * Create fetch structure to hold the fetch attribute unique identifier
   */
  struct mailimap_fetch_type* fetchStruct = mailimap_fetch_type_new_fetch_att_list_empty();

  struct mailimap_fetch_att* fetchAtt = mailimap_fetch_att_new_uid();
  
  clist* MessageList;

  std::string ErrMsg = "Failed to retrieve messages.";

  check_error(mailimap_fetch_type_new_fetch_att_list_add(fetchStruct, fetchAtt), ErrMsg);

  check_error(mailimap_fetch(imap, set, fetchStruct, &MessageList), ErrMsg);
  
  /**
   * For each message, obtain UID from the list of attributes of the clist element
   */
  int count = 0;
  
  for ( clistiter* msg_iter = clist_begin(MessageList); msg_iter != nullptr; msg_iter = clist_next(msg_iter) ) {
    
    auto msg_att = (struct mailimap_msg_att*) clist_content(msg_iter);    
    auto uid = getUID(msg_att);

    if (uid) {
  
      /**
       * Modify instance of Message class passing on Session and message
       *
       * information through the implicit pointer: this and its UID
       */
      messages[count] = new Message(this, uid);
         
      count++;
    }
  }

  messages[count] = nullptr;
  
  /**
   * Deallocate the necessary memory
   */
  
  mailimap_fetch_list_free(MessageList);
  mailimap_fetch_type_free(fetchStruct);
  mailimap_set_free(set);
  
  return messages;
}


void Session::connect(std::string const& server, size_t port){

  std::string ErrMsg = "No server connection.";

  check_error(mailimap_socket_connect(imap, server.c_str(), port), ErrMsg);
}

void Session::login(std::string const& userid, std::string const& password){

  std::string ErrMsg = "Login Error.";

  check_error(mailimap_login(imap, userid.c_str(), password.c_str()), ErrMsg);
}


void Session::selectMailbox(std::string const& Mailbox){

  /**
   * Save mailbox name (used to retrieve number of messages in mailbox)
   */
  mailbox = Mailbox;
  
  std::string ErrMsg = "Unable to select the specified mailbox.";

  check_error(mailimap_select(imap, Mailbox.c_str()), ErrMsg);
}

