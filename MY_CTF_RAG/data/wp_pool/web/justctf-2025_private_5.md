# justctf-2025_private~5

# Vault Tec

Cloud(AWS) based challenge

AWS dependent parts:
1. Admin app EC2
2. Subnet of admin app EC2. ID Should be passed as `ADMIN_SUBNET` environment variable in `challenges/web_vault_tec/src/webapp/compose.yaml` 
3. IAM role assumed by webapp. ARN should be passed as `ROLE_ARN` environment variable in `challenges/web_vault_tec/src/webapp/compose.yaml`
4. IAM user used by vault. Credentials should be passed as `AWS_ACCESS_KEY` and `AWS_SECRET_KEY` environment variable in `challenges/web_vault_tec/src/webapp/compose.yaml`
It should have following permissions:

```
data "aws_iam_policy_document" "vault_policy" {
  statement {
    actions   = ["ec2:DescribeInstances", "iam:GetRole"]
    effect    = "Allow"
    resources = ["*"]
  }
}
```
