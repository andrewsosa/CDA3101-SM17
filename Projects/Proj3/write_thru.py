# write_thru.py

from cache import Cache, Block

class WriteThru(Cache):

    def read(self, addr):
        hit, set_ = self.hit(addr)

        if hit:
            """
            When a block is present in the cache (hit), a read simply grabs the data for the processor.
            """
            self.__report__.hits += 1

            """
            Also update the block's age.
            """
            # Find block in set
            block = next((x for x in set_ if x.tag == self.tag(addr)), None)

            # Error check
            if block is None:
                print "A problem occured..."
                exit()

            # Update age
            block.age = self.counter


        else:
            """
            When a block is not present in the cache (miss), a read will cause the block to be pulled from main memory into the cache, replacing the least recently used block if necessary.
            """
            self.__report__.misses += 1
            self.__report__.mem_refs += 1

            # If full, we replace
            if not any(block.tag == -1 for block in set_):
                # Find the oldest block
                block = min(set_, key=lambda x: x.age)
                i = set_.index(block)

                # Overwrite slot with new block
                block = Block(self.counter, self.tag(addr))
                set_[i] = block

            # Otherwise, find out empty spot
            else:
                # Prep the new block
                new_block = Block(self.counter, self.tag(addr))

                # Locate an empty spot
                old_block = next((x for x in set_ if x.tag == -1), None)

                # Make sure we're OK
                if old_block is None:
                    print "A problem occured..."
                    exit()

                # Insert new block
                i = set_.index(old_block)
                set_[i] = new_block


    def write(self, addr):
        hit, set_ = self.hit(addr)

        if hit:
            """
            When a block is present in the cache (hit), a write will update both the cache and main memory (i.e. we are writing through to main memory).
            """
            self.__report__.hits += 1
            self.__report__.mem_refs += 1

            """
            Also update the block's age.
            """
            # Find block in set
            block = next((x for x in set_ if x.tag == self.tag(addr)), None)

            # Error check
            if block is None:
                print "A problem occured..."
                exit()

            # Update age
            block.age = self.counter


        else:
            """
            When a block is not present in the cache (miss), a write will update the block in main memory but we do not bring the block into the cache (this is why it is called no write allocate).
            """
            self.__report__.misses += 1
            self.__report__.mem_refs += 1
