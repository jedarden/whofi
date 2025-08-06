#!/bin/bash
# Deploy GitHub Actions Runner to apexalgo-iad cluster

set -e

echo "=== Deploying GitHub Actions Runner for whofi to apexalgo-iad cluster ==="

# Check if kubectl is configured
if ! kubectl cluster-info &> /dev/null; then
    echo "❌ kubectl is not configured or cluster is not accessible"
    echo "Please ensure you have the apexalgo-iad kubeconfig set up"
    exit 1
fi

# Check if arc-systems namespace exists
if ! kubectl get namespace arc-systems &> /dev/null; then
    echo "❌ arc-systems namespace not found"
    echo "Please ensure Actions Runner Controller is installed first"
    echo "Follow the installation at: https://github.com/actions/actions-runner-controller"
    exit 1
fi

# Create namespace if it doesn't exist
echo "📦 Creating github-actions-runners namespace..."
kubectl apply -f namespace.yaml

# Check if secret already exists
if kubectl get secret whofi-github-secret -n github-actions-runners &> /dev/null; then
    echo "⚠️  Secret whofi-github-secret already exists"
    echo "   To update it, delete the existing secret first:"
    echo "   kubectl delete secret whofi-github-secret -n github-actions-runners"
else
    echo "🔐 Creating GitHub secret..."
    echo "   Please edit whofi-github-secret.yaml with your GitHub App credentials or PAT"
    echo "   Then run: kubectl apply -f whofi-github-secret.yaml"
fi

# Apply the runner application
echo "🚀 Deploying runner application..."
kubectl apply -f whofi-runner-application.yaml

# Wait for deployment
echo "⏳ Waiting for runners to be ready..."
kubectl wait --for=condition=ready pod -l runner-deployment-name=whofi-apexalgo-iad-runners -n github-actions-runners --timeout=300s || true

# Check status
echo "📊 Checking runner status..."
kubectl get pods -n github-actions-runners -l runner-deployment-name=whofi-apexalgo-iad-runners

echo ""
echo "✅ Deployment complete!"
echo ""
echo "Next steps:"
echo "1. Configure your GitHub secret in whofi-github-secret.yaml"
echo "2. Apply the secret: kubectl apply -f whofi-github-secret.yaml"
echo "3. Check runner registration at: https://github.com/jedarden/whofi/settings/actions/runners"
echo "4. Monitor runners: kubectl logs -n github-actions-runners -l runner-deployment-name=whofi-apexalgo-iad-runners"
echo ""
echo "To use these runners in your workflow, add:"
echo "  runs-on: [self-hosted, kubernetes, whofi]"