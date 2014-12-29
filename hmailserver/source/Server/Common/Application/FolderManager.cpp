// Copyright (c) 2010 Martin Knafve / hMailServer.com.  
// http://www.hmailserver.com

#include "StdAfx.h"

#include "FolderManager.h"
#include "../BO/Account.h"
#include "../BO/IMAPFolders.h"
#include "../BO/IMAPFolder.h"

#include "../../IMAP/IMAPFolderContainer.h"
#include "../../IMAP/IMAPConfiguration.h"
#include "../../IMAP/IMAPFolderUtilities.h"

#include "../Tracking/ChangeNotification.h"
#include "../Tracking/NotificationServer.h"

#ifdef _DEBUG
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

namespace HM
{
   FolderManager::FolderManager(void)
   {

   }

   FolderManager::~FolderManager(void)
   {

   }

   bool
   FolderManager::GetInboxMessages(int accountID, std::vector<std::shared_ptr<Message>> &result)
   {
      std::shared_ptr<IMAPFolders> folders = IMAPFolderContainer::Instance()->GetFoldersForAccount(accountID);
      if (!folders)
         return false;

      std::shared_ptr<IMAPFolder> inbox = folders->GetItemByName("Inbox");
      if (!inbox)
         return false;

      // get folder lock.
      result = inbox->GetMessages()->GetCopy();

      return true;
   }

   bool 
   FolderManager::DeleteInboxMessages(int accountID, std::set<int> uids, const std::function<void()> &func)
   {
      std::shared_ptr<IMAPFolders> folders = IMAPFolderContainer::Instance()->GetFoldersForAccount(accountID);
      if (!folders)
         return false;

      std::shared_ptr<IMAPFolder> folder = folders->GetFolderByName("Inbox", false);
      if (!folder)
         return false;

      __int64 inboxID = folder->GetID();

      std::vector<int> expungedMessages = folder->Expunge(uids, func);
      
      if (expungedMessages.size() > 0)
      {
         std::vector<__int64> affectedMessages;
         for(__int64 messageIndex : expungedMessages)
         {
            affectedMessages.push_back(messageIndex);
         }

         // Notify the mailbox notifier that the mailbox contents have changed.
         std::shared_ptr<ChangeNotification> pNotification = 
            std::shared_ptr<ChangeNotification>(new ChangeNotification(accountID, inboxID, ChangeNotification::NotificationMessageDeleted, affectedMessages));

         Application::Instance()->GetNotificationServer()->SendNotification(pNotification);
      }


      return true;
   }

   bool 
   FolderManager::UpdateMessageFlags(int accountID, int folderID, __int64 messageID, int flags)
   {
      std::shared_ptr<IMAPFolders> folders;
      if (accountID > 0)
         folders = IMAPFolderContainer::Instance()->GetFoldersForAccount(accountID);
      else
         folders = IMAPFolderContainer::Instance()->GetPublicFolders();

      if (!folders)
         return false;

      std::shared_ptr<IMAPFolder> folder = folders->GetItemByDBIDRecursive(folderID);
      if (!folder)
         return false;

      std::shared_ptr<Message> message = folder->GetMessages()->GetItemByDBID(messageID);

      if (!message)
         return false;

      message->SetFlags(flags);
      return PersistentMessage::SaveFlags(message);
   }


   
} 