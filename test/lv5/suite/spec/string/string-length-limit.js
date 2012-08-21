describe("String", function() {
  describe("length limit", function() {
    it("repeat", function() {
      expect(function() {
        '000'.repeat(0x7FFFFFFF)
      }).toThrow();
    });
  });
});
