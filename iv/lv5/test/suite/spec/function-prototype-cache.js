describe("Function", function() {
  it("prototype cache #91", function() {
    function Dummy() { }

    function inherit(a, b) {
      Dummy.prototype = b.prototype;
      a.prototype = new Dummy();
      a.prototype.constructor = a;
    }

    function A() { }
    function B() { }
    function C() { }
    function D() { }

    inherit(C, A);
    inherit(D, B);

    expect(Object.getPrototypeOf(C.prototype)).toBe(A.prototype);
    expect(Object.getPrototypeOf(D.prototype)).toBe(B.prototype);
  });
});
