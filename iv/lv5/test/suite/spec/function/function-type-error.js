describe("Function", function() {
  describe('TypeError', function() {
    it("assignment", function() {
      expect(function f() { 'use strict'; f = 0; }).toThrow();
    });
  });
});
