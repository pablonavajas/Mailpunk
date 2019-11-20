#include "imap.hpp"

using namespace IMAP;
using namespace std;


Session::Session(std::function<void()> updateUI): SessNewUI(updateUI)
{  
  imap = mailimap_new(0,nullptr);
  messages = nullptr;
  NoMessages = 0;
  mailbox = "";
}

Session::~Session(){
  
  mailimap_logout(imap);
  mailimap_free(imap);
}


Message::Message(Session* Session, int UID, std::function<void()> SessNewUI): MsgNewUI(SessNewUI)
{
  body = "";
  subject = "";
  from = "";

  uid = UID;

  session = Session;


  mailimap_set* set = mailimap_set_new_single(uid);

  struct mailimap_section * section = mailimap_section_new(nullptr);
  
  struct mailimap_fetch_type* fetchStruct = mailimap_fetch_type_new_fetch_att_list_empty();

  clist * contents;


  //header:

  auto headers_att = mailimap_fetch_att_new_envelope();

  mailimap_fetch_type_new_fetch_att_list_add(fetchStruct, headers_att);


  //body:
  
  auto body_att = mailimap_fetch_att_new_body_section(section);

  mailimap_fetch_type_new_fetch_att_list_add(fetchStruct, body_att);
  

  std::string ErrMsg = "Unable to retrieve message";
  
  check_error( mailimap_uid_fetch(session->imap, set, fetchStruct, &contents), ErrMsg);


  clistiter* Listidx = clist_begin(contents);

  auto msg_att = (struct mailimap_msg_att*)clist_content(Listidx);

  getAtts(msg_att);
  
  
  mailimap_fetch_list_free(contents);
  
  mailimap_fetch_type_free(fetchStruct);

  mailimap_set_free(set);
}






void Message::getAtts(struct mailimap_msg_att* msg_att) {

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

  return body;
}





std::string Message::getField(std::string fieldname){

  if (fieldname == "Subject")
    return subject;

  else if (fieldname == "From")
    return from;

}


void Message::deleteFromMailbox(){

  struct mailimap_set* set = mailimap_set_new_single(uid);

  struct mailimap_flag_list* DelFlagList = mailimap_flag_list_new_empty();

  struct mailimap_flag* DelFlag = mailimap_flag_new_deleted();

  std::string ErrMsg = "Error deleting message";
  
  check_error( mailimap_flag_list_add(DelFlagList, DelFlag), ErrMsg);


  struct mailimap_store_att_flags* store_att = mailimap_store_att_flags_new_set_flags(DelFlagList);

  check_error( mailimap_uid_store(session->imap, set, store_att), ErrMsg);


  check_error( mailimap_expunge(session->imap), ErrMsg);

    
  store_att = mailimap_store_att_flags_new(store_att->fl_sign,store_att->fl_silent,DelFlagList);

  mailimap_set_free(set);

  
  MsgNewUI();
}



void Session::CountMessages() {
  
  struct mailimap_status_att_list* StatusAttList = mailimap_status_att_list_new_empty();

  mailimap_status_att_list_add(StatusAttList, MAILIMAP_STATUS_ATT_MESSAGES);

  mailimap_mailbox_data_status* StatusData;

  std::string ErrMsg = "Unable to obtain No of Messages";

  check_error(mailimap_status(imap, mailbox.c_str(), StatusAttList, &StatusData), ErrMsg);

  NoMessages =((struct mailimap_status_info*) clist_content(clist_begin(StatusData->st_info_list)))->st_value;
  
  mailimap_mailbox_data_status_free(StatusData);
  mailimap_status_att_list_free(StatusAttList);
}



int Session::getUID(mailimap_msg_att* msg_att) {

  for ( clistiter* att_iter = clist_begin(msg_att->att_list); att_iter != nullptr; att_iter = clist_next(att_iter) ) {

    auto item = (struct mailimap_msg_att_item*) clist_content(att_iter);

    if ( item->att_type == MAILIMAP_MSG_ATT_ITEM_STATIC and item->att_data.att_static->att_type == MAILIMAP_MSG_ATT_UID )
      return item->att_data.att_static->att_data.att_uid;

    return 0;
  }
}




Message** Session::getMessages(){

  
  CountMessages();

  messages = new Message*[NoMessages + 1];

  struct mailimap_set* set = mailimap_set_new_interval(1,0);

  struct mailimap_fetch_type* fetchStruct = mailimap_fetch_type_new_fetch_att_list_empty();

  struct mailimap_fetch_att* fetchAtt = mailimap_fetch_att_new_uid();

  clist* MessageList;

  std::string ErrMsg = "Failed to retrieve messages";


  check_error(mailimap_fetch_type_new_fetch_att_list_add(fetchStruct, fetchAtt), ErrMsg);

  check_error(mailimap_fetch(imap, set, fetchStruct, &MessageList), ErrMsg);

  
  size_t count = 0;
  
  for ( clistiter* msg_iter = clist_begin(MessageList); msg_iter != nullptr; msg_iter = clist_next(msg_iter) ) {
    
    auto msg_att = (struct mailimap_msg_att*) clist_content(msg_iter);

    
    auto uid = getUID(msg_att);

    if (uid) {
      
      messages[count] = new Message(this, uid, SessNewUI);
         
      count++;
    }
  }
  
  messages[count] = nullptr;


  mailimap_fetch_list_free(MessageList);
  mailimap_fetch_type_free(fetchStruct);
  mailimap_set_free(set);
  

  return messages;


  //return new Message*[5]{};
}


void Session::connect(std::string const& server, size_t port){

  std::string ErrMsg = "no server connection";

  check_error(mailimap_socket_connect(imap, server.c_str(), port), ErrMsg);
}

void Session::login(std::string const& userid, std::string const& password){

  std::string ErrMsg = "unable to log in";

  check_error(mailimap_login(imap, userid.c_str(), password.c_str()), ErrMsg);
}

void Session::selectMailbox(std::string const& Mailbox){

  mailbox = Mailbox;
  
  std::string ErrMsg = "unable to select mailbox";

  check_error(mailimap_select(imap, Mailbox.c_str()), ErrMsg);
}

