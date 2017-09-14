// (C) 2017 University of Bristol. See License.txt

#ifndef _Exceptions
#define _Exceptions

#include <exception>
#include <string>
#include <sstream>
using namespace std;

class not_implemented: public exception
    { virtual const char* what() const throw()
        { return "Case not implemented"; }
    };
class division_by_zero: public exception
    { virtual const char* what() const throw()
        { return "Division by zero"; }
    };
class invalid_plaintext: public exception
    { virtual const char* what() const throw()
        { return "Inconsistent plaintext space"; }
    };
class rep_mismatch: public exception
    { virtual const char* what() const throw()
        { return "Representation mismatch"; }
    };
class pr_mismatch: public exception
    { virtual const char* what() const throw()
        { return "Prime mismatch"; }
    };
class params_mismatch: public exception
    { virtual const char* what() const throw()
        { return "FHE params mismatch"; }
    };
class field_mismatch: public exception
    { virtual const char* what() const throw()
        { return "Plaintext Field mismatch"; }
    };
class level_mismatch: public exception
    { virtual const char* what() const throw()
        { return "Level mismatch"; }
    };
class invalid_length: public exception
    { virtual const char* what() const throw()
        { return "Invalid length"; }
    };
class invalid_commitment: public exception
    { virtual const char* what() const throw()
        { return "Invalid Commitment"; }
    };
class IO_Error: public exception
    { string msg, ans;
      public:
      IO_Error(string m) : msg(m)
        { ans="IO-Error : ";
          ans+=msg;
        }
      ~IO_Error()throw() { }
      virtual const char* what() const throw()
        {
          return ans.c_str(); 
        }
    };
class broadcast_invalid: public exception
    { virtual const char* what() const throw()
        { return "Inconsistent broadcast at some point"; }
    };
class bad_keygen: public exception
    { string msg;
      public:
      bad_keygen(string m) : msg(m) {}
      ~bad_keygen()throw() { }
      virtual const char* what() const throw()
          { string ans="KeyGen has gone wrong: "+msg; 
            return ans.c_str();
          }
    };
class bad_enccommit:  public exception
    { virtual const char* what() const throw()
        { return "Error in EncCommit"; }
    };
class invalid_params:  public exception
    { virtual const char* what() const throw()
        { return "Invalid Params"; }
    };
class bad_value: public exception
    { virtual const char* what() const throw()
        { return "Some value is wrong somewhere"; }
    };
class Offline_Check_Error: public exception
    { string msg;
      public:
      Offline_Check_Error(string m) : msg(m) {}
      ~Offline_Check_Error()throw() { }
      virtual const char* what() const throw()
        { string ans="Offline-Check-Error : ";
          ans+=msg;
          return ans.c_str();
        }
    };
class mac_fail:  public exception
    { virtual const char* what() const throw()
        { return "MacCheck Failure"; }
    };
class invalid_program:  public exception
    { virtual const char* what() const throw()
        { return "Invalid Program"; }
    };
class file_error:  public exception
    { string filename, ans;
      public:
      file_error(string m="") : filename(m)
        {
          ans="File Error : ";
          ans+=filename;
        }
      ~file_error()throw() { }
      virtual const char* what() const throw()
        {
          return ans.c_str();
        }
    };
class end_of_file: public exception
    { string filename, context, ans;
      public:
      end_of_file(string pfilename="no filename", string pcontext="") : 
        filename(pfilename), context(pcontext)
        {
          ans="End of file when reading ";
          ans+=filename;
          ans+=" ";
          ans+=context;
        }
      ~end_of_file()throw() { }
      virtual const char* what() const throw()
        {
          return ans.c_str();
        }
    };
class file_missing:  public exception
    { string filename, context, ans;
      public:
      file_missing(string pfilename="no filename", string pcontext="") : 
        filename(pfilename), context(pcontext)
        {
          ans="File missing : ";
          ans+=filename;
          ans+=" ";
          ans+=context;
        }
      ~file_missing()throw() { }
      virtual const char* what() const throw()
        {
          return ans.c_str();
        }
    };    
class Processor_Error: public exception
    { string msg;
      public:
      Processor_Error(string m)
        {
          msg = "Processor-Error : " + m;
        }
      ~Processor_Error()throw() { }
      virtual const char* what() const throw()
        {
          return msg.c_str();
        }
    };
class Invalid_Instruction : public Processor_Error
    {
      public:
      Invalid_Instruction(string m) : Processor_Error(m) {}
    };
class max_mod_sz_too_small : public exception
    { int len;
      public:
      max_mod_sz_too_small(int len) : len(len) {}
      ~max_mod_sz_too_small() throw() {}
      virtual const char* what() const throw()
        { stringstream out;
          out << "MAX_MOD_SZ too small for desired bit length of p, "
                  << "must be at least ceil(len(p)/len(word))+1, "
                  << "in this case: " << len;
          return out.str().c_str();
        }
    };
class crash_requested: public exception
    { virtual const char* what() const throw()
        { return "Crash requested by program"; }
    };
class memory_exception : public exception {};
class how_would_that_work : public exception {};



#endif
