describe("Map", function() {
  describe('clear', function() {
    it('with string', function() {
      var map = new Map();
      map.set('0', 0);
      expect(map.size()).toBe(1);
      map.clear();
      expect(map.size()).toBe(0);
    });
  });
});
