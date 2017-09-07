# report.py

class Report:
    """ Modular record of cache access.
    """

    def __init__(self):
        self.refs = 0
        self.hits = 0
        self.misses = 0
        self.mem_refs = 0

    def print_report(self, name):
        report = """

****************************************
{0}
****************************************
Total number of references: {1}
Hits: {2}
Misses: {3}
Memory References: {4}

        """.format(name, self.refs, self.hits, self.misses, self.mem_refs)

        print report
