# GitHub Actions Runner on Kubernetes

This directory contains the Kubernetes configurations to deploy self-hosted GitHub Actions runners for the whofi repository on the apexalgo-iad cluster.

## Overview

Self-hosted runners allow you to:
- Run CI/CD jobs on your own infrastructure
- Access cluster resources directly
- Use custom tools and dependencies
- Reduce GitHub Actions minutes usage
- Have better control over the runner environment

## Setup Options

### Option 1: Basic Runner Deployment (Simple)

This uses a standard deployment with the summerwind/actions-runner image.

```bash
# 1. Create namespace
kubectl apply -f namespace.yaml

# 2. Create secret with your GitHub tokens
# First, get a runner token from: https://github.com/jedarden/whofi/settings/actions/runners
kubectl create secret generic github-runner-secret \
  --from-literal=GITHUB_TOKEN=<your-github-pat> \
  --from-literal=REPO_URL=https://github.com/jedarden/whofi \
  --from-literal=RUNNER_TOKEN=<your-runner-token> \
  -n github-actions-runners

# 3. Deploy the runners
kubectl apply -f runner-deployment.yaml

# 4. Verify runners are registered
kubectl logs -n github-actions-runners -l app=github-runner
```

### Option 2: Actions Runner Controller (Recommended for Production)

This uses the official Actions Runner Controller (ARC) for better management and auto-scaling.

```bash
# 1. Install cert-manager (required by ARC)
kubectl apply -f https://github.com/cert-manager/cert-manager/releases/download/v1.13.0/cert-manager.yaml

# 2. Wait for cert-manager to be ready
kubectl wait --for=condition=ready pod -l app.kubernetes.io/instance=cert-manager -n cert-manager --timeout=180s

# 3. Install Actions Runner Controller
kubectl apply -f https://github.com/actions/actions-runner-controller/releases/download/v0.27.6/actions-runner-controller.yaml

# 4. Create GitHub App or PAT secret for ARC
kubectl create secret generic controller-manager \
  --from-literal=github_token=<your-github-pat> \
  -n actions-runner-system

# 5. Deploy the runner configuration
kubectl apply -f runner-controller.yaml
```

## Configuration Details

### Runner Labels

The runners are configured with these labels:
- `self-hosted`
- `linux`
- `x64`
- `apexalgo-iad`
- `kubernetes`
- `whofi`

You can target these runners in your workflows:

```yaml
jobs:
  build:
    runs-on: [self-hosted, kubernetes, whofi]
    steps:
      - uses: actions/checkout@v3
      - name: Build
        run: |
          echo "Running on self-hosted Kubernetes runner"
```

### Resource Limits

Default resource configuration:
- **Requests**: 512Mi memory, 500m CPU
- **Limits**: 2Gi memory, 2 CPU

Adjust these in `runner-deployment.yaml` based on your workload requirements.

### Docker-in-Docker Support

The runners are configured with Docker socket access for building containers:
- Mounts `/var/run/docker.sock` from host
- Enables building and pushing Docker images
- Use with caution in production environments

## Security Considerations

1. **Token Security**:
   - Use GitHub Apps instead of PATs when possible
   - Rotate tokens regularly
   - Use fine-grained permissions

2. **Network Policies**:
   ```yaml
   apiVersion: networking.k8s.io/v1
   kind: NetworkPolicy
   metadata:
     name: github-runner-network-policy
     namespace: github-actions-runners
   spec:
     podSelector:
       matchLabels:
         app: github-runner
     policyTypes:
     - Ingress
     - Egress
     egress:
     - to:
       - namespaceSelector: {}
       ports:
       - protocol: TCP
         port: 443  # HTTPS for GitHub API
       - protocol: TCP
         port: 80   # HTTP (some package managers)
   ```

3. **Pod Security Standards**:
   - Consider using restricted security contexts
   - Avoid running as root when possible
   - Use read-only root filesystems where applicable

## Monitoring

Check runner status:
```bash
# List all runners
kubectl get pods -n github-actions-runners

# Check runner logs
kubectl logs -n github-actions-runners -l app=github-runner --tail=100

# Check runner registration
kubectl exec -n github-actions-runners <runner-pod> -- ./config.sh --check
```

## Troubleshooting

### Runner not appearing in GitHub

1. Check the logs for registration errors:
   ```bash
   kubectl logs -n github-actions-runners -l app=github-runner | grep -i error
   ```

2. Verify the secret is correct:
   ```bash
   kubectl get secret github-runner-secret -n github-actions-runners -o yaml
   ```

3. Ensure the runner token is valid (they expire after 1 hour if unused)

### Runner keeps restarting

1. Check resource limits - the runner might be OOMKilled
2. Verify Docker socket access if building containers
3. Check for network connectivity to GitHub

### Performance issues

1. Increase resource limits in the deployment
2. Use node affinity to run on specific nodes
3. Consider using persistent volumes for build caches

## Advanced Configuration

### Using with Cluster Resources

To allow runners to interact with the cluster:

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: github-runner-cluster-access
rules:
- apiGroups: ["*"]
  resources: ["*"]
  verbs: ["get", "list", "watch"]  # Read-only access
```

### Build Caching

Add persistent volume for build caches:

```yaml
volumes:
- name: build-cache
  persistentVolumeClaim:
    claimName: runner-build-cache
volumeMounts:
- name: build-cache
  mountPath: /runner/.cache
```

## Example Workflow

Here's an example workflow that uses the self-hosted runner:

```yaml
name: Build and Deploy

on:
  push:
    branches: [main]

jobs:
  build:
    runs-on: [self-hosted, kubernetes, whofi]
    steps:
    - uses: actions/checkout@v3
    
    - name: Build Docker image
      run: |
        docker build -t whofi-app:${{ github.sha }} .
    
    - name: Deploy to cluster
      run: |
        kubectl set image deployment/whofi-app \
          whofi-app=whofi-app:${{ github.sha }} \
          -n whofi-production
```

## Maintenance

- Regularly update the runner image for security patches
- Monitor runner performance and adjust resources as needed
- Clean up old runner pods that might be stuck
- Review and rotate GitHub tokens periodically