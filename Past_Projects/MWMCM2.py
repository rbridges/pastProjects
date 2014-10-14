import sys
import operator
from copy import deepcopy


infile = open(sys.argv[1], "r")
outfile = open(sys.argv[1].replace("in","ot"),"w")
#infile = open("tests/file1.in","r")

# ---------------- Utilities, etc ------------------#
def rangeBound(lis, lowest, highest):
    for element in lis:
        if element>=lowest and element<= highest:
            yield element
        if element > highest:
            break

class Tic:
    tid = -1
    Min = -1
    Max = -1
    weight = -9999999
    def __init__(self, lis):
        self.tid = int(lis[0])  
        self.Min = int(lis[1])
        self.Max = int(lis[2])
        self.weight = int(lis[3])
        self.preferredTacs = []
        self.Mine = -1
    def __str__(self):
        return "("+str(self.id) + ", "+str(self.Min)+", "+str(self.Max)+", "+str(self.weight)+")"

class Tac:
    tad = -1
    weight = -9999999
    def __init__(self, lis):
        self.tad = int(lis[0])
        self.weight = int(lis[1])
    def __str__(self):
        return "("+str(self.tad)+", "+str(self.weight)+")"
    

# -------------------- parse input file ----------------------- #
numGraphs = infile.readline()
graphs = []
for i in range(int(numGraphs)):
    tidList = [] #tics id list
    tadList = [] #tacs id list
    ticsd = {}
    tacsd = {}
    quantities = infile.readline().split()
    for j in range ( int(quantities[0]) ):
        line = infile.readline().split()
        tic = Tic(line)
        tidList.append(tic.tid)
        ticsd[tic.tid] = tic #tics ht for quick access
    for k in range ( int(quantities[1]) ):
        line = infile.readline().split()
        tac = Tac(line)
        tadList.append(tac.tad)
        tacsd[tac.tad] = tac #tacs ht
        
    tidList.sort()
    tadList.sort()
    graphs.append( [tidList,tadList,ticsd,tacsd] )


# -------------- Brute Force  -------------- #


class State:
    tidList = [] #tics id list
    tadList = [] #tacs id list
    matchingsList = []
    totalWeight = 0
    def __init__(self,ticsl, tacsl, ticsd, tacsd):
        self.tidList = ticsl
        self.tadList = tacsl
        self.matchingsList = []
        self.totalWeight = 0
    def addMatch(self, tic, tac):
        combinedWeight = ticsd[tic].weight + tacsd[tac].weight
        self.matchingsList.append( (tic,tac,combinedWeight))
       # print("matchingsList: " + str(self.matchingsList))
        self.totalWeight += combinedWeight
    def retirePair(self, tic, tac):
        self.tidList.remove(tic)
        self.tadList.remove(tac)
    def __str__(self):
        string = str(self.totalWeight) + ":\n\t"
        return string + str(self.matchingsList)

def export(state,leaderboard):
  #  print("this state: " + str(state)+"\n")
  #  print("leaderboard states: \n")
  #  for ste in leaderboard:
   #     print("lb st: " + str(ste)+"\n")
    
    if len(leaderboard)==0:
        leaderboard.append(state)
    elif state.totalWeight < leaderboard[0].totalWeight:
        return leaderboard
    elif (state.totalWeight > leaderboard[0].totalWeight):
        del leaderboard[:]
    #print("returned leaderboard of size: " + str(len(leaderboard)))
    leaderboard.append(state)
    return leaderboard


def BF(state, leaderboard):
    if len(state.tadList) == 0 or len(state.tidList)==0:
        leaderboard = export(state,leaderboard)
        return leaderboard
    for tic in state.tidList:
        least = ticsd[tic].Min
        greatest = ticsd[tic].Max
        for tac in rangeBound(state.tadList,least,greatest):
            state_copy = deepcopy(state)
            state_copy.addMatch(tic, tac)
            state_copy.retirePair(tic, tac)
            leaderboard = BF(state_copy,leaderboard)
    return leaderboard

def contains(compareAgainst, candidate):
    for tupl in candidate:
        yield (tupl in compareAgainst)

def swap(lis, i):
    temp = lis[i]
    lis[i] = lis[i+1]
    lis[i+1] = temp

def firstGreater(lis1, lis2):
    i = 0
    while(i<=len(lis1)):
        if lis1[i] == lis2[i]:
            i+=1
            continue
        if lis1[i] > lis2[i]:
            return True
        else:
            return False
    return False
def bubblesortByTacs(listOtuples):
    swapMade = True
    while(swapMade):
        swapMade = False
        for i in range( len(listOtuples) ):
            if i == len(listOtuples)-1:
                break
            #print("i: " +str(i))
            #print("len: " +str( len(listOtuples)))
            if listOtuples[i][1]>listOtuples[i+1][1]:
                swapMade = True
                swap(listOtuples, i)

def bubblesortByTics(matchings):
    swapMade = True
    while(swapMade):
        swapMade = False
        for i in range( len(matchings)):
            if i == len(matchings)-1:
                break
            if firstGreater(matchings[i],matchings[i+1]):
                swapMade = True
                swap(matchings, i)
    
for items in graphs:
    tidList = items[0] #tics id list
    tadList = items[1] #tacs id list
    ticsd = items[2]   #tics dictionary
    tacsd = items[3] #tacs dictionary
    leaderboard = [] #this will hold the top MWMCMs
    initial_state = State(tidList, tadList, ticsd, tacsd)
    items.append(leaderboard)
    leaderboard = BF(initial_state, leaderboard)

    ### get rid of the permutations of the answer set
    lines = []
    for state in leaderboard:
        matches = set()
        for tupl in state.matchingsList:
            matches.add(tupl)
        lines.append(matches)

    use = []
    for i in range(len(lines)):
        use.append(True) 
    for i in range(len(lines)):
        for j in range(i+1, len(lines)):
            if j > len(lines):
                break
            if all( contains(lines[i],lines[j])):
                use[j] = False
    
    MWMCMsets = [] # this finally contains a list of the sets of MWMCMs
    for i in range(len(lines)):
        if use[i]:
            MWMCMsets.append(lines[i])


    ### order the matchings properly for output
    tacOrdered = []
    for s in MWMCMsets:
        matching = list(s)
        bubblesortByTacs(matching)
        tacOrdered.append(matching)
    #print(tacOrdered)

    bubblesortByTics(tacOrdered)
    outfile.write(str(len(tacOrdered)) +"\n")
    for line in tacOrdered:
        for tupl in line:
            outfile.write(str(tupl[0])+":"+str(tupl[1])+" ")
        outfile.write("\n")
