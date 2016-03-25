#ifndef Visitor_hpp
#define Visitor_hpp

namespace RTX {
  class BaseVisitor {
  public:
    virtual ~BaseVisitor() {};
  };
  
  template <class T, typename R = int>
  class Visitor {
  public:
    virtual R visit(T &) = 0;
  };
  
  template <typename R = int>
  class BaseVisitable {
  public:
    typedef R ReturnType;
    virtual ~BaseVisitable() {};
    virtual ReturnType accept(BaseVisitor & ) {
      return ReturnType(0);
    }
  protected:
    template <class T>
    static ReturnType acceptVisitor(T &visited, BaseVisitor &visitor) {
      if (Visitor<T> *p = dynamic_cast< Visitor<T> *> (&visitor)) {
        return p->visit(visited);
      }
      return ReturnType(-1);
    }
    
  #define VISITABLE() virtual ReturnType accept(BaseVisitor &v) { return acceptVisitor(*this, v); }
  };

}

#endif /* Visitor_hpp */
