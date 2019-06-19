DOCKER_IMAGE_NAME = 'gcr.io/organic-storm-201412/fetch-ledger-develop:v0.3.0'
HIGH_LOAD_NODE_LABEL = 'ledger'
MACOS_NODE_LABEL = 'mac-mini'

enum Platform
{
  DEFAULT_CLANG('Clang',      'clang',     'clang++'    ),
  CLANG6       ('Clang 6',    'clang-6.0', 'clang++-6.0'),
  CLANG7       ('Clang 7',    'clang-7',   'clang++-7'  ),
  GCC7         ('GCC 7',      'gcc-7',     'g++-7'      ),
  GCC8         ('GCC 8',      'gcc-8',     'g++-8'      )

  public Platform(label, cc, cxx)
  {
    this.label = label
    this.env_cc = cc
    this.env_cxx = cxx
  }

  public final String label
  public final String env_cc
  public final String env_cxx
}

LINUX_PLATFORMS = [
  Platform.CLANG6,
  Platform.CLANG7,
  Platform.GCC7,
  Platform.GCC8]

enum Configuration
{
  DEBUG  ('Debug'),
  RELEASE('Release')

  public Configuration(label)
  {
    this.label = label
  }

  public final String label
}

// Only execute long-running tests on master and merge branches
def is_master_or_merge_branch()
{
  return BRANCH_NAME == 'master' || BRANCH_NAME ==~ /^PR-\d+-merge$/
}

def static_analysis()
{
  return {
    stage('Static Analysis') {
      node {
        stage('SCM Static Analysis') {
          checkout scm
        }
                        
        stage('Run Static Analysis') {
          docker.image(DOCKER_IMAGE_NAME).inside {
            sh '''\
              mkdir -p build-analysis
              cd build-analysis
              cmake ..
            '''
            sh './scripts/run_static_analysis.py build-analysis'
          }
        }
      }
    }
  }
}

def _create_build(
  Platform platform,
  Configuration config,
  node_label,
  run_build_fn,
  run_slow_tests)
{
  def suffix = "${platform.label} ${config.label}"

  def environment = {
    return [
      "CC=${platform.env_cc}",
      "CXX=${platform.env_cxx}"
    ]
  }

  def stages = {
    stage("Build ${suffix}") {
      sh "./scripts/ci-tool.py -B ${config.label}"
    }

    stage("Unit Tests ${suffix}") {
      sh "./scripts/ci-tool.py -T ${config.label}"
    }

    if (run_slow_tests)
    {
      stage("Slow Tests ${suffix}") {
        sh "./scripts/ci-tool.py -S ${config.label}"
      }

      stage("Integration Tests ${suffix}") {
        sh "./scripts/ci-tool.py -I ${config.label}"
      }

      stage("End-to-End Tests ${suffix}") {
        sh './scripts/ci/install-test-dependencies.sh'
        sh "./scripts/ci-tool.py -E ${config.label}"
      }
    }
  }

  return {
    stage(suffix) {
      node(node_label) {
        timeout(120) {
          stage("SCM ${suffix}") {
            checkout scm
          }

          withEnv(environment()) {
            run_build_fn(stages)
          }
        }
      }
    }
  }
}

def create_docker_build(Platform platform, Configuration config)
{
  def dockerised_build = { build_stages ->
    docker.image(DOCKER_IMAGE_NAME).inside {
      build_stages()
    }
  }

  return _create_build(
    platform,
    config,
    HIGH_LOAD_NODE_LABEL,
    dockerised_build,
    is_master_or_merge_branch())
}

def create_macos_build(Platform platform, Configuration config)
{
  return _create_build(
    platform,
    config,
    MACOS_NODE_LABEL,
    { build_stages -> build_stages() },
    false)
}

def run_builds_in_parallel()
{
  def stages = [:]

  for (config in Configuration.values())
  {
    for (platform in LINUX_PLATFORMS)
    {
      stages["${platform.label} ${config.label}"] = create_docker_build(
        platform,
        config)
    }
  }

  // Only run macOS builds on master and merge branches
  if (is_master_or_merge_branch())
  {
    stages["macOS Clang Release"] = create_macos_build(
      Platform.DEFAULT_CLANG,
      Configuration.RELEASE)
  }

  stages['Static Analysis'] = static_analysis()

  stage('Build and Test') {
    // Execute stages
    parallel stages
  }
}

def run_basic_checks()
{
  stage('Basic Checks') {
    node {
      stage('SCM Basic Checks') {
        checkout scm
      }

      docker.image(DOCKER_IMAGE_NAME).inside {
        stage('Style Check') {
          sh './scripts/apply_style.py -d'
        }
      }
    }
  }
}

def main()
{
  timeout(180) {
    run_basic_checks()
    run_builds_in_parallel()
  }
}

// Entry point
main()
