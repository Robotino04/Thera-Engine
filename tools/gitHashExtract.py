import sys
try:
    import git
except(ImportError):
    raise Exception("\n\npython git isn't installed. Execute: pip install git-python\n\n")

repo = git.Repo(search_parent_directories=True)
sha = repo.head.object.hexsha

template_file = sys.argv[1]
output_file = sys.argv[2]

with open(template_file, "r") as f:
    template = f.read()

template = template.format(
    is_dirty = str(repo.is_dirty()).lower(),
    sha=sha
)

with open(output_file, "w") as f:
    f.write(template)