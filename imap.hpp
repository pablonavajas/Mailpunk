#ifndef IMAP_H
#define IMAP_H
#include "imaputils.hpp"
#include <libetpan/libetpan.h>
#include <string>
#include <functional>



namespace IMAP {

  class Session;
  
  class Message {
    
  public:

    int uid;

    Session* session;
    
    std::string body;

    std::string subject;

    std::string from;

    std::function<void()> MsgNewUI;

    Message(Session* Session, int UID, std::function<void()> updateUI);

    
    
    void normform(clist * fromlist);
    
    void getAtts(struct mailimap_msg_att* msg_att);
    

    /**
     * Get the body of the message. You may chose to either include the headers or not.
     */   
    //void getMessageBody(clist * contents);
    std::string getBody();
    /**
     * Get one of the descriptor fields (subject, from, ...)
     */
    std::string getField(std::string fieldname);
    /**
     * Remove this mail from its mailbox
     */
    void deleteFromMailbox();
  };
  
  class Session {

  public:
    
    int NoMessages;

    std::string mailbox;
    
    struct mailimap * imap;

    Message** messages;

    std::function<void()> SessNewUI;
    
    Session(std::function<void()> updateUI);
    
    ~Session();
    
    void CountMessages();

    int getUID(mailimap_msg_att* msg_att);
    
    /**
     * Get all messages in the INBOX mailbox terminated by a nullptr (like we did in class)
     */
    
    Message** getMessages();
    
    /**
     * connect to the specified server (143 is the standard unencrypte imap port)
     */
    
    void connect(std::string const& server, size_t port = 143);
    
    /**
     * log in to the server (connect first, then log in)
     */
    void login(std::string const& userid, std::string const& password);
    
    /**
     * select a mailbox (only one can be selected at any given time)
     * 
     * this can only be performed after login
     */
    void selectMailbox(std::string const& Mailbox);
    
  };
}

#endif /* IMAP_H */


//(echo From: $USER@`hostname`; echo "Subject: A test email"; echo; echo "Here is the first line of the body";echo "and the second") | curl smtp://mailpunk.lsds.uk --mail-rcpt ${USER}mail@localhost -T -
//USER=pn319mail SERVER=mailpunk.lsds.uk PASSWORD=42617446 ./MailPunk
