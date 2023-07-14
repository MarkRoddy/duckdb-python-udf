
import os, sys
from github import Github
from ducktables import ducktable

token = os.environ.get("GITHUB_ACCESS_TOKEN")
g = Github(token)

@ducktable(repo = str, description = str, language = str)
def repos_for(username):
    for r in g.get_user(username).get_repos():
        yield (r.name, r.description, r.language)


@ducktable(id = int, name = str, path = str, state = str, created_at = str, updated_at = str, url = str)
def workflows(repo_name):
    repo = g.get_repo(repo_name)
    for w in repo.get_workflows():
        yield (w.id, w.name, w.path, w.state, w.created_at.isoformat(sep='T',timespec='auto'), w.updated_at.isoformat(sep='T',timespec='auto'), w.url)

@ducktable(id = int, workflow_name = str, head_branch = str, head_sha = str, workflow_path = str, run_attempt = str,
           run_number = str, event = str, run_started_at = str, status = str, conclusion = str, workflow_id = int,
           url = str, created_at = str, updated_at = str, jobs_url = str, logs_url = str, check_suite_url = str,
           artifacts_url = str)
def workflow_runs(repo_name):
    # The WorkflowRun object doesn't contain information about the workflow itself. So
    # here we use a Dict to cache the workflow objects themselves so we don't end up
    # with a GET request for every run enumerated. Especially given the ratio of workflows
    # to runs is so huge.
    workflows = {}
    repo = g.get_repo(repo_name)
    for r in repo.get_workflow_runs():

        # Do a cached lookup of the workflow associated with the run
        if not r.workflow_id in workflows:
            workflows[r.workflow_id] = repo.get_workflow(r.workflow_id)
        w = workflows[r.workflow_id]

        # Yield our DB record
        yield (r.id, w.name, r.head_branch, r.head_sha, w.path, r.run_attempt, r.run_number, r.event,
               r.run_started_at.isoformat(sep='T',timespec='auto'), r.status, r.conclusion, r.workflow_id,
               r.url, r.created_at.isoformat(sep='T',timespec='auto'), r.updated_at.isoformat(sep='T',timespec='auto'),
               r.jobs_url, r.logs_url, r.check_suite_url, r.artifacts_url)
        

def main(args):
    for i, r in enumerate(repos_for('markroddy')):
        if i > 10:
            break
        else:
            print(r)

    for w in workflows("MarkRoddy/duckdb-pytables"):
        print(w)

    for i, r in enumerate(workflow_runs("MarkRoddy/duckdb-pytables")):
        if i > 10:
            break
        else:
            print(r)
    
if __name__ == '__main__':
    sys.exit(main(sys.argv))
