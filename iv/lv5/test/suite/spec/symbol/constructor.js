describe("Symbol", function() {
  describe('constructor called with new', function() {
    it("with no arguments", function() {
      expect(function() {
        var symbol = new Symbol();
      }).toThrow();
    });

    it("with undefined", function() {
      expect(function() {
        var symbol = new Symbol(undefined);
      }).toThrow();
    });

    it("with null", function() {
      expect(function() {
        var symbol = new Symbol(null);
      }).toThrow();
    });

    it("with string", function() {
      expect(function() {
        var symbol = new Symbol("HELLO");
      }).toThrow();
    });
  });

  describe('constructor called', function() {
    it("with no arguments", function() {
      expect(function() {
        var symbol = Symbol();
      }).not.toThrow();
    });

    it("with undefined", function() {
      expect(function() {
        var symbol = Symbol(undefined);
      }).not.toThrow();
    });

    it("with null", function() {
      expect(function() {
        var symbol = Symbol(null);
      }).not.toThrow();
    });

    it("with string", function() {
      expect(function() {
        var symbol = Symbol("HELLO");
      }).not.toThrow();
    });
  });
});
/* vim: set sw=2 ts=2 et tw=80 : */
