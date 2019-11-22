//Imperial College London - Department of Computing
//MSc Computing CO157 Object-Oriented Design & Programming
//Assessed Exercise No. 1
//Mailpunk
//
//Created by: Pablo Navajas Helguero
//Date: 20th November 2019
//
//File: imap.hpp


#ifndef IMAP_H
#define IMAP_H
#include "imaputils.hpp"
#include <libetpan/libetpan.h>
#include <string>
#include <functional>


namespace IMAP {

  /**
   * class Foward declaration to use the type "Session" in Message class
   */
  class Session; 


  class Message {

    /**
     * Data member to keep information on the Session   
     */
    Session* session;

    /**
     * Data members holding message information
     */
    int uid;
    
    std::string msg_cont;
    /**
     * Declare Session class as friend to access message information (uid)
     */    
    friend class Session;
    
    /**
     * Lambda without parameter or return to call updateUI
     */
    std::function<void()> updateUI;

    /*
     * Member functions to retrieve attributes data
     */
    void getAtts(clist* contents);

    void extractfield(const std::string & field);
    
  public:

    Message(Session* Session, int UID);
    /**
     * Get the body of the message. You may chose to either include the headers or not.
     */   
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

    int NoMessages;

    std::string mailbox;
    
    struct mailimap * imap;

    Message** messages;

    /**
     * Declare Message class as friend to access session information (imap)
     */    
    friend class Message;
    
    /**
     * Lambda without parameter or return to call updateUI
     */    
    std::function<void()> updateUI;

    /**
     * Member functions to retrieve Messages data
     */
    void CountMessages();                   //Number of mesages
    
    int getUID(mailimap_msg_att* msg_att);  //Unique identifier

    /**
     * Member function to delete all messages avoiding memory leak
     */
    void deleteExcept(int msg_uid);
    
  public:

    /**
     * Constructor declaration
     */
    Session(std::function<void()> updateUI);

    /**
     * Destructor declaration
     */
    ~Session();

    /**
     * Get all messages in the INBOX mailbox terminated by a nullptr (like we did in class)
     */
    Message** getMessages();

    /**
     * connect to the specified server (143 is the standard unencrypte imap port)
     *
     * Using default argument for port 
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
