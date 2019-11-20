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

using namespace IMAP;

/**
 * In-line constructor definition for updateUI function
 */
Message::Message(Session* Session, int UID, std::function<void()> SessNewUI): MsgNewUI(SessNewUI)
{
  /**
   * Initialise Mesage data members in constructor
   */
  body = "";
  subject = "";
  from = "";
  uid = UID;

  session = Session;

  /**
   * Create set to fetch a single message, fetch structure
   * 
   * to retrieve specific contents
   */
  mailimap_set* set = mailimap_set_new_single(uid);
  
  struct mailimap_fetch_type* fetchStruct = mailimap_fetch_type_new_fetch_att_list_empty();

  clist * contents;

  /**
   * Add headers attribute to fetching list
   */
  auto headers_att = mailimap_fetch_att_new_envelope();

  mailimap_fetch_type_new_fetch_att_list_add(fetchStruct, headers_att);

  /**
   * Add body attribute to fetching list
   */
  struct mailimap_section * bodySection = mailimap_section_new(nullptr);
  
  auto body_att = mailimap_fetch_att_new_body_section(bodySection);

  mailimap_fetch_type_new_fetch_att_list_add(fetchStruct, body_att);
  
  /**
   * Retrieve attributes data from the selected message
   */
  std::string ErrMsg = "Unable to retrieve message";
  
  check_error( mailimap_uid_fetch(session->imap, set, fetchStruct, &contents), ErrMsg);

  /**
   * Get the list of attributes from the first clist and get specified attributtes
   */
  auto msg_att = (struct mailimap_msg_att*)clist_content(clist_begin(contents));

  getAtts(msg_att);
  
  /**
   * Deallocate specified memory
   */
  mailimap_fetch_list_free(contents);
  
  mailimap_fetch_type_free(fetchStruct);

  mailimap_set_free(set);
}


void Message::getAtts(struct mailimap_msg_att* msg_att) {

  /**
   * For each attribute in the list, check if it is 
   * 
   * the body, the subject or the "from" field
   */
  for (clistiter* attidx = clist_begin(msg_att->att_list) ; attidx != nullptr ; attidx = clist_next(attidx) ) {

    auto item = (struct mailimap_msg_att_item*) clist_content(attidx);

    if (item->att_type == MAILIMAP_MSG_ATT_ITEM_STATIC) {

      if (item->att_data.att_static->att_type == MAILIMAP_MSG_ATT_BODY_SECTION)
	body = item->att_data.att_static->att_data.att_body_section->sec_body_part;

      else if (item->att_data.att_static->att_type == MAILIMAP_MSG_ATT_ENVELOPE) {

	subject = item->att_data.att_static->att_data.att_env->env_subject;

	normform(item->att_data.att_static->att_data.att_env->env_from->frm_list);
      }
    }
  }   
}


void Message::normform(clist * fromlist) {

  /**
   * For each attribute in the list, check if it has 
   * 
   * mailbox and host, and build "from" field
   */
  for (clistiter * fromidx = clist_begin(fromlist) ; fromidx != nullptr ; fromidx = clist_next(fromidx) ) {

    auto email = (mailimap_address*) clist_content(fromidx);

    if (email->ad_mailbox_name and email->ad_host_name) {

      from += email->ad_mailbox_name;
      from += '@';
      from += email->ad_host_name;
      from += " ; ";
    }
  }
}


std::string Message::getBody(){
  /**
   * Display retrieved body content
   */
  return body;
}


std::string Message::getField(std::string fieldname){

  /**
   * Display retrieved header data
   */
  if (fieldname == "Subject")
    return subject;

  else if (fieldname == "From")
    return from;

}


void Message::deleteFromMailbox(){
  
  /**
   * Create set to fetch a single message, list of flags and \Deleted flag object
   */
  struct mailimap_set* set = mailimap_set_new_single(uid);

  struct mailimap_flag_list* DelFlagList = mailimap_flag_list_new_empty();

  struct mailimap_flag* DelFlag = mailimap_flag_new_deleted();
  
  /**
   * Add flag to list of flags
   */
  std::string ErrMsg = "Error deleting message";
  
  check_error( mailimap_flag_list_add(DelFlagList, DelFlag), ErrMsg);

  /**
   * Change message flags (mark as \Deleted)
   */
  struct mailimap_store_att_flags* store_att = mailimap_store_att_flags_new_set_flags(DelFlagList);

  check_error( mailimap_uid_store(session->imap, set, store_att), ErrMsg);

  /**
   * Permanently remove it from the mailbox
   */
  check_error( mailimap_expunge(session->imap), ErrMsg);

  /**
   * Deallocate the necessary memory
   */
  store_att = mailimap_store_att_flags_new(store_att->fl_sign,store_att->fl_silent,DelFlagList);

  mailimap_set_free(set);

  /**
   * Call for UI update
   */
  MsgNewUI();
}



/**
 * In-line constructor definition for updateUI function
 */
Session::Session(std::function<void()> updateUI): SessNewUI(updateUI)
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

  /**
   * Get the number of messages value from the first clist 
   *
   * (from "useful snippets" section)
   */
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


Message** Session::getMessages(){

  /**
   * Obtain number of messages and 
   * 
   * initialise the required number of messages instances
   */
  CountMessages();

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

  /**
   * Create a clist to hold message attribute information
   */
  clist* MessageList;

  std::string ErrMsg = "Failed to retrieve messages.";

  /**
   * Add the attribute to the fetch list (or raise error)
   * 
   * retrieve message data of the given attribute (or raise error)
   */
  check_error(mailimap_fetch_type_new_fetch_att_list_add(fetchStruct, fetchAtt), ErrMsg);

  check_error(mailimap_fetch(imap, set, fetchStruct, &MessageList), ErrMsg);
  
  /**
   * For each message, obtain UID from the list of attributes of the clist element
   */
  size_t count = 0;
  
  for ( clistiter* msg_iter = clist_begin(MessageList); msg_iter != nullptr; msg_iter = clist_next(msg_iter) ) {
    
    auto msg_att = (struct mailimap_msg_att*) clist_content(msg_iter);    
    auto uid = getUID(msg_att);

    if (uid) {
 
      /**
       * Modify instance of Message class passing on Session and message
       *
       * information through the implicit pointer: this and its UID
       */
      messages[count] = new Message(this, uid, SessNewUI);
         
      count++;
    }
  }

  /**
   * Add null pointer to end messages list
   */
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

  /**
   * Establish Socket (or raise error)
   */
  std::string ErrMsg = "No server connection.";

  check_error(mailimap_socket_connect(imap, server.c_str(), port), ErrMsg);
}

void Session::login(std::string const& userid, std::string const& password){

  /**
   * Log in (or raise error)
   */
  std::string ErrMsg = "Login Error.";

  check_error(mailimap_login(imap, userid.c_str(), password.c_str()), ErrMsg);
}


void Session::selectMailbox(std::string const& Mailbox){

  /**
   * Save mailbox name (used to retrieve number of messages in mailbox)
   * 
   * Select mailbox (or raise error)
   */
  mailbox = Mailbox;
  
  std::string ErrMsg = "Unable to select the specified mailbox.";

  check_error(mailimap_select(imap, Mailbox.c_str()), ErrMsg);
}

