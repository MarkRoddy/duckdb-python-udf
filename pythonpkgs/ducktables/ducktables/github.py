
import os, sys
from github import Github
from ducktables import ducktable

token = os.environ.get("GITHUB_ACCESS_TOKEN")
g = Github(token)

@ducktable(repo = str, description = str, language = str)
def repos_for(username):
    for r in g.get_user(username).get_repos():
        yield (r.name, r.description, r.language)

def main(args):
    for r in repos_for('markroddy'):
        print(r)

if __name__ == '__main__':
    sys.exit(main(sys.argv))
