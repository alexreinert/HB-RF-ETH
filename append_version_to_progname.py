Import("env")

with open("version.txt") as fp:
    version = fp.readline()
    env.Replace(PROGNAME="firmware_%s" % version.replace(".", "_"))
