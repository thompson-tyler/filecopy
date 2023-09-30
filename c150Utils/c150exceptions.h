// --------------------------------------------------------------
//
//                        c150exceptions.h
//
//   
//        Standard exceptions for use in Tufts COMP 150-IDS
//
//    Author: Noah Mendelsohn         
//
//    Students will be provided with a utility framework that will
//    catch these exceptions and provide useful debugging information.
//
//    Also, it is intended that networking utility code provided
//    for students will throw exceptions.
//
//    NEEDSWORK: for historical reasons, because some early
//    programs used the namespace, two copies are provided
//    of each of these exceptions, one in the C150NETWORK namespace,
//    and one un-namespaced. Should be converged in no namespace.
//    only network exceptions should be in the namespace.
//     
//        Copyright: 2012 Noah Mendelsohn
//     
// --------------------------------------------------------------

#ifndef __C150EXCEPTIONS_H_INCLUDED__ 
#define __C150EXCEPTIONS_H_INCLUDED__ 


#include <string>

using namespace std;


// ***********************************************************
//                NAMESPACED VERSIONS
// ***********************************************************

namespace C150NETWORK {


  //
  //   C150Exception
  //
  //   All COMP 150 exceptions derive from this base class, so
  //   you can "catch" this to get them all. It includes
  //   a standard provision for conveying a string explanation
  //   of the error.
  //
  //   DERIVING OTHER EXCEPTION CLASSES FROM THIS ONE
  //   ==============================================
  //
  //   Derived exception classes should use the 2 argument
  //   constructor to override the exception name. For most uses, this
  //   works better than overriding the exceptionName() method, because
  //   a catch clause catching the base name (C150Exception) gets a copy
  //   of the exception without all the derived stuff (see C++ rules for
  //   catching exceptions).
  //   
  //   For the same reason, derived exceptions are encouraged to provide as much useful
  //   information as possible in the explanation set in the constructor, though it is
  //   possible to also override formattedExplanation to provide addtional
  //   information. The overridden method will be visible only to catch clauses that explicitly
  //   name the derived exception class.
  //   
  //
  class C150Exception {
  private:
    string explain;
  protected:
    string classname;
    // Derived exception classes should use this constructor, which allows
    // them to set the true name of the exception
    C150Exception(string exceptionName, string s) : explain(s), classname(exceptionName) {};
  public:
    C150Exception(string s) : explain(s), classname("C150Exception") {};
    C150Exception() : explain("C150Exception thrown with no explanation given") {}; 

    // override this to provide customized explanations
    virtual string exceptionName() {return classname;}

    // get just the explanation as a string
    virtual string explanation() {return explain;}

    // formatted explanation includes addition information such as the exception name
    virtual string formattedExplanation() {return exceptionName() + ":  " + explanation();}

    virtual ~C150Exception() {};
  };

  //
  //   C150ConversionException
  //
  //   Error doing type conversion
  //
  class C150ConversionException : public C150Exception {
    private:
      string input;
    public:
      // constructor: takes just an explanation
      C150ConversionException(string explain) : C150Exception("C150ConversionException",explain), input("") {};
      // Use this constructor if you also have available a string
      // representing the input that caused the error
      C150ConversionException(string explain, string inp) : C150Exception("C150ConversionException",explain), input(inp) {};
      
      // Use this constructor if you're really too lazy to provide an explanation (discouraged)
      C150ConversionException() : C150Exception("C150ConversionException", "C150ConversionException thrown with no explanation given"), input("") {};

      // Override formatted explanation to include input string
	virtual string formattedExplanation() {return C150Exception::formattedExplanation() + " -- " + "input text=" + getInput();}
      // accessor to for input string causing exception ("" if not provided)
      inline string getInput() {return input;}

      // best to have virtual destructor for classes with virtual methods
      virtual ~C150ConversionException() {};
    
  };
}

#ifdef NONAMESPACE

// ***********************************************************
//                UN-NAMESPACED VERSIONS
// ***********************************************************

  //
  //   C150Exception
  //
  //   All COMP 150 exceptions derive from this base class, so
  //   you can "catch" this to get them all. It includes
  //   a standard provision for conveying a string explanation
  //   of the error.
  //
  //   DERIVING OTHER EXCEPTION CLASSES FROM THIS ONE
  //   ==============================================
  //
  //   Derived exception classes should use the 2 argument
  //   constructor to override the exception name. For most uses, this
  //   works better than overriding the exceptionName() method, because
  //   a catch clause catching the base name (C150Exception) gets a copy
  //   of the exception without all the derived stuff (see C++ rules for
  //   catching exceptions).
  //   
  //   For the same reason, derived exceptions are encouraged to provide as much useful
  //   information as possible in the explanation set in the constructor, though it is
  //   possible to also override formattedExplanation to provide addtional
  //   information. The overridden method will be visible only to catch clauses that explicitly
  //   name the derived exception class.
  //   
  //
  class C150Exception {
  private:
    string explain;
  protected:
    string classname;
    // Derived exception classes should use this constructor, which allows
    // them to set the true name of the exception
    C150Exception(string exceptionName, string s) : explain(s), classname(exceptionName) {};
  public:
    C150Exception(string s) : explain(s), classname("C150Exception") {};
    C150Exception() : explain("C150Exception thrown with no explanation given") {}; 

    // override this to provide customized explanations
    virtual string exceptionName() {return classname;}

    // get just the explanation as a string
    virtual string explanation() {return explain;}

    // formatted explanation includes addition information such as the exception name
    virtual string formattedExplanation() {return exceptionName() + ":  " + explanation();}

    virtual ~C150Exception() {};
  };

  //
  //   C150ConversionException
  //
  //   Error doing type conversion
  //
  class C150ConversionException : public C150Exception {
    private:
      string input;
    public:
      // constructor: takes just an explanation
      C150ConversionException(string explain) : C150Exception("C150ConversionException",explain), input("") {};
      // Use this constructor if you also have available a string
      // representing the input that caused the error
      C150ConversionException(string explain, string inp) : C150Exception("C150ConversionException",explain), input(inp) {};
      
      // Use this constructor if you're really too lazy to provide an explanation (discouraged)
      C150ConversionException() : C150Exception("C150ConversionException", "C150ConversionException thrown with no explanation given"), input("") {};

      // Override formatted explanation to include input string
	virtual string formattedExplanation() {return C150Exception::formattedExplanation() + " -- " + "input text=" + getInput();}
      // accessor to for input string causing exception ("" if not provided)
      inline string getInput() {return input;}

      // best to have virtual destructor for classes with virtual methods
      virtual ~C150ConversionException() {};
    
  };

#endif
#endif


