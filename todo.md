- Find meson command line option to put install prefix in environment variable
- // look at ADTs again - linked list generic ADT?
// make ADT struct of value, next where value is a pointer to the details of the
// User / blockedUser?
- BlockedUsersList: make the rwlock write preferred
- Commands.c: // if implement as reading through a list of loggedin users, WOULD need
// lock? new user could be added (noone can ever be removed) to tail while
// reading. If before tail, doesn't matter. Could determine that cur->next =
// null but then it changes to not null before exit. Doesn't sound bad? BUT it
// does matter if something is partially added and then this tries to read it eg
// if tail->next has been set but the values inside not instantiated but I could
// just avoid that by only setting tail->next when the next is fully set up 

### Potential improvements
- BlockedUsersList: could be improved with some data structure sorted by blockee name
- BlockedUsersList: checkBlocked: keep sorted list (or something more fancy???) of blockees and do binary search?
- generic ADT for messageList, blockedUsersList?. Clients also very similar to blockedUsers
- rather than passing around user pointers, I could have a users array and pass around indexes
- 
