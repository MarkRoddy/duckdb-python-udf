# DuckTables: Python Functions for DuckDB

DuckTables is Python library that provides out of the box functions for the [DuckDB PyTables](https://github.com/MarkRoddy/duckdb-pytables) extension.


# Installation

```shell
pip install ducktables
```

# Authentication
DuckTables interacts with a number of different services, and each has their own authentication and configuration mechanism. Note that you only have to configure the services that you plan on using.

## AWS
Follow the [auth instructions](https://boto3.amazonaws.com/v1/documentation/api/latest/guide/quickstart.html#configuration) for Boto3, the library that ducktables uses to interface with AWS apis. Note that if you're already using boto3 or the AWS CLI, your existing authentication configuration will be utilized, and you don't need to do anything.

## Github
When starting your DuckDB client (such as the DuckDB cli), set an environment variable named `GITHUB_ACCESS_TOKEN`. From the command line, this will look like:

```shell
GITHUB_ACCESS_TOKEN=<token> duckdb -unsigned
```

To obtain an API token, log into github.com, and create one [here](https://github.com/settings/tokens).

## OpenAI
To use OpenAI functions you will need a valid API key and organization id. If you're logged in, you can create an API key [here](https://platform.openai.com/account/api-keys), and your organization id can be found [here](https://platform.openai.com/account/org-settings).

To specify these values, you'll need to set two environment variables when running DuckDB: `OPENAI_ORG_ID` and `OPENAI_API_KEY`. This may look something like:
```shell
OPENAI_ORG_ID=<orid> OPENAI_API_KEY=<apikey> duckdb -unsigned
```

## Google
Google related functions assume you have a json applications credentials file on disk where you're running DuckDB. The user or service account assoicated with the file will need to have the necessary permissions to interact with services associated with any functions you may use.

To specify the path to the credentials file, set an evironment variable named `GOOGLE_APPLICATION_CREDENTIALS` when running DuckDB. That may look something like:
```shell
GOOGLE_APPLICATION_CREDENTIALS=~/.gcloud/some-file.json duckdb -unsigned
```

# Example Queries
These queries assume you've already installed the [PyTables](https://github.com/MarkRoddy/duckdb-pytables) extension in a DuckDB session. See the extensions [installation guide](https://github.com/MarkRoddy/duckdb-pytables#installation) if you have not.

## AWS
Note that all queries will assume to run in the region you have specified in your configuration file. Set the `AWS_DEFAULT_REGION` to override this behavior.
### EC2 Instances
Returns the results of a [DescribeInstances](https://docs.aws.amazon.com/AWSEC2/latest/APIReference/API_DescribeInstances.html) API call.
```sql
    SELECT * FROM pytable('ducktables.aws:ec2_instances');
```

### S3 Buckets
Returns the results of a [ListBuckets](https://docs.aws.amazon.com/AmazonS3/latest/API/API_ListBuckets.html) API call.
```SQL
SELECT * FROM pytable('ducktables.aws:s3_buckets');
```

### S3 Objects
This only includes metadata about the objects themselves, not their contents. See the [httpfs](https://duckdb.org/docs/guides/import/s3_import.html) DuckDB extension if you're interested in loading data from objects on S3.

Note that the second argument, the prefix path, is optional and can be omitted.
```SQL
SELECT * FROM pytable('ducktables.aws:s3_objects', 'bucket-name', 'foo/bar/prefix');
```

## Github
Enumerates all repositories for the named user or organization.
```SQL
SELECT * FROM pytable('ducktables.githb:repos_for', 'duckdb');
```

## Open AI

### ChatGPT
Given a prompt and a number of desired responses, will generate a table with the model's response.
```SQL
SELECT * FROM pytable('chatgpt:prompt', 'Write a limerick poem about how much you love SQL', 2,
  columns = {'index': 'INT','message': 'VARCHAR', 'message_role': 'VARCHAR', 'finish_reason': 'VARCHAR'},
);
```

## Google

### Sheets
Pulls data from a Google Sheet. Note that you'll either need user account authorization, or if you use a service account, the
account will need read permissions on the sheet in question if it is private.

This example query pulls from a Google provided example spreadsheet. You can see the sheet [here](https://docs.google.com/spreadsheets/d/1BxiMVs0XRA5nFMdKvBdBZjgmUUqptlbs74OgvE2upms/edit#gid=0). Note that despite this sheet being public, you will still need Google Cloud authorization configured on your local machine.
```SQL
SELECT *
FROM pytable('google:sheet', '1BxiMVs0XRA5nFMdKvBdBZjgmUUqptlbs74OgvE2upms', 'Class Data!A2:F31',
  columns = {
    'name': 'VARCHAR', 'gender': 'VARCHAR', 'class_level': 'VARCHAR',
    'state': 'VARCHAR', 'major': 'VARCHAR', 'extracurricular': 'VARCHAR'}
);
```

### Analytics
This query has three required parameters. The ViewID you wish to query, and the start/end date range for your query. See [this post](https://stackoverflow.com/a/47921777) for an explanation of finding your view ID. Start and end dates should be strings in a 'YYYY-MM-DD' format. Note you can alos override the set of Dimensions and Metrics, but this requires understanding how to adjust the `columns` argument to account for the change in schema that will occur as a result.
```SQL
    SELECT * FROM pytable('ducktables.google.analytics:metrics', '<view-id>', '<start-date>', '<end-date>',
      columns = {
        -- Dimensions
        'date': 'VARCHAR', 'hour': 'VARCHAR', 'pagePath': 'VARCHAR', 'source': 'VARCHAR', 'medium': 'VARCHAR',
        -- Metrics
        'sessions': 'INT', 'users': 'INT', 'pageviews': 'INT', 'avgSessionDuration': 'FLOAT', 'bounceRate': 'FLOAT'
        });
```

