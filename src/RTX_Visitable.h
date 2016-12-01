#ifndef RTX_Visitable_hpp
#define RTX_Visitable_hpp

namespace RTX {
  class BaseVisitor {
  public:
    virtual ~BaseVisitor() {};
  };
  
  template <class T>
  class Visitor {
  public:
    virtual void visit(T&) = 0;
  };
  
  class BaseVisitable {
  public:
    virtual ~BaseVisitable() {};
    virtual void accept(BaseVisitor & ) { }
  protected:
    template <class T>
    static void acceptVisitor(T &visited, BaseVisitor &visitor) {
      if (Visitor<T> *p = dynamic_cast< Visitor<T> *> (&visitor)) {
        return p->visit(visited);
      }
    }
  };
  
  #define RTX_VISITABLE() virtual void accept(BaseVisitor &v) { return acceptVisitor(*this, v); }
  #define RTX_VISITABLE_TYPE BaseVisitable
}

#endif /* RTX_Visitable_hpp */
