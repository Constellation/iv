describe("Set", function() {
  describe('constructor', function() {
    it("with no arguments", function() {
      expect(function() {
        var set = new Set();
      }).not.toThrow();
    });

    it("with undefined", function() {
      expect(function() {
        var set = new Set(undefined);
      }).not.toThrow();
    });

    it("with null", function() {
      expect(function() {
        var set = new Set(null);
      }).toThrow();
    });

    it("with number", function() {
      expect(function() {
        var set = new Set(0);
      }).toThrow();
      expect(function() {
        var set = new Set(-0);
      }).toThrow();
      expect(function() {
        var set = new Set(NaN);
      }).toThrow();
    });

    it("with string", function() {
      expect(function() {
        var set = new Set('a');
      }).toThrow();
      expect(function() {
        var set = new Set('abc');
      }).toThrow();
    });

    it("with boolean", function() {
      expect(function() {
        var set = new Set(true);
      }).toThrow();
      expect(function() {
        var set = new Set(false);
      }).toThrow();
    });

    it("with initializer", function() {
      var set = new Set([0, -0, '0']);
      expect(set.has(0)).toBe(true);
      expect(set.has(-0)).toBe(true);
      expect(set.has('0')).toBe(true);
    });

    it("with empty array", function() {
      expect(function() {
        new Set([]);
      }).not.toThrow();
    });
  });
});
